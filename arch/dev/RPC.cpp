#include "RPC.h"
#include "config.h"

/*

MDS = metadata service
ODS = object data service

The RPC device is pipelined.


The input to the pipeline is a tuple:
- device ID for DCA (ID of requestor); channel ID for completion notification
- procedure ID
- completion writeback (payload in the completion notification)
- address in memory where the arguments are to be fetched from
- memory size for the argument data
- address in memory where the results are to be stored

The completion writeback and memory addresses are passive registers;
the command is issued with their current value when the "command
action register" is written to.

Internally; for each command issued:

- the argument data is fetched from the I/O core's memory via DCA
- the command is performed
- the results are sent back via DCA
- the notification is sent back.

The amount of data transferred from the I/O core's memory is
predefined in the command; but the internal format is dependent on the
command type (more below; expect something RPC-ish).

Queues:
- incoming queue: commands issued via the I/O port
- ready queue: commands after the argument data has been fetched
- completed queue: results that need to be communicated back to memory
- notification queue: for completion notifications

The following processes run concurrently (pipeline):

- argument fetch: issues DCA reead requests until the argument for the
  front incoming request has been read, then deactivate until the last read
  completes; then queue the complete reauest to the processing queue;
  then pop the front incoming request.

- processing: reads in requests from the processing queue; dispatches
  the behavior, then queues the result to the completion queue.

- result writeback: issues DCA write reauests until the result data
  has been sent to memory; then send a read request of 0 as a memory barrier; 
  then deactivate until the MB completes; then pop the front incoming request.

- notifications:  issues completion notification for the frontmost
  notification entry. Then pop the entry.

  - read request handler:
    informs the latches for all parameters except command type
    informs the current size of the queues for monitoring
    informs the current state of the processes for monitoring

  - write reauest handler:
    stores values for the parameters into latches
    initiates commands by enqueuing into the incoming queue

  - read response handler: 

    if the size is greater than 0; then populate the argument data for
    the frontmost entry in the incoming queue.  If there are no pending read
    requests; then wake up the argument fetch process

    if the size is 0, this is the end of a memory barrier; flag the
    frontmost entry in the completion queue. (will be picked up by the
    result writeback process)

*/

// FIXME: because the input buffer has a fixed size, why use DCA? the
// issuing core could be in charge of populating the input. -- NO: DCA
// has more throughput! entire cache lines transferred at once.

// FIXME: completion n otifications are sent to all I/O cores and
// are queued on each core: all cores not interested in a given
// channel must have a thread that flushes the queue, otherwise
// the I/O network will stall.

namespace Simulator
{

    RPCInterface::RPCInterface(const std::string& name, Object& parent, IIOBus& iobus, IODeviceID devid, Config& config, IRPCServiceProvider& provider)
        : Object(name, parent, iobus.GetClock()),
          m_iobus(iobus),
          m_devid(devid),

          m_lineSize(config.getValueOrDefault<size_t>(*this, "RPCLineSize", config.getValue<size_t>("CacheLineSize"))),

          m_fetchState(ARGFETCH_READING1),
          m_currentArgumentOffset(0),
          m_numPendingDCAReads(0),
          m_currentArgData1(config.getValue<size_t>(*this, "RPCBufferSize1"), 0),
          m_currentArgData2(config.getValue<size_t>(*this, "RPCBufferSize2"), 0),

          m_writebackState(RESULTWB_WRITING1),
          m_currentResponseOffset(0),

          m_queueEnabled ("f_queue",         *this, iobus.GetClock(), false),
          m_incoming     ("b_incoming",      *this, iobus.GetClock(), config.getValue<BufferSize>(*this, "RPCIncomingQueueSize")),
          m_ready        ("b_ready",         *this, iobus.GetClock(), config.getValue<BufferSize>(*this, "RPCReadyQueueSize")),
          m_completed    ("b_completed",     *this, iobus.GetClock(), config.getValue<BufferSize>(*this, "RPCCompletedQueueSize")),
          m_notifications("b_notifications", *this, iobus.GetClock(), config.getValue<BufferSize>(*this, "RPCNotificationQueueSize")),

          m_provider(provider),

          p_queue                      (*this, "queue-request",                 delegate::create<RPCInterface, &RPCInterface::DoQueue>(*this)),
          p_argumentFetch              (*this, "fetch-argument-data",           delegate::create<RPCInterface, &RPCInterface::DoArgumentFetch>(*this)),
          p_processRequests            (*this, "process-requests",              delegate::create<RPCInterface, &RPCInterface::DoProcessRequests>(*this)),
          p_writeResponse              (*this, "write-response",                delegate::create<RPCInterface, &RPCInterface::DoWriteResponse>(*this)),
          p_sendCompletionNotifications(*this, "send-completion-notifications", delegate::create<RPCInterface, &RPCInterface::DoSendCompletionNotifications>(*this))
    {
        iobus.RegisterClient(devid, *this);

        m_incoming.Sensitive(p_argumentFetch);
        m_ready.Sensitive(p_processRequests);
        m_completed.Sensitive(p_writeResponse);
        m_notifications.Sensitive(p_sendCompletionNotifications);

        if (m_lineSize == 0)
        {
            throw exceptf<InvalidArgumentException>(*this, "RPCLineSize cannot be zero");
        }        
        
    }

    Result RPCInterface::DoQueue()
    {
        if (!m_incoming.Push(m_inputLatch))
        {
            DeadlockWrite("Unable to push incoming request");
            return FAILED;
        }
        m_queueEnabled.Clear();
        return SUCCESS;
    }

    Result RPCInterface::DoArgumentFetch()
    {
        assert(!m_incoming.Empty());

        const IncomingRequest& req = m_incoming.Front();
        ArgumentFetchState s = m_fetchState;        

        if (s == ARGFETCH_READING1 && (req.argres1_base_address == 0 || req.argres1_numwords == 0))
        {
            s = (req.argres2_base_address == 0 || req.argres2_numwords == 0) ? ARGFETCH_FINALIZE : ARGFETCH_READING2;
        }
        else if (s == ARGFETCH_READING2 && (req.argres2_base_address == 0 || req.argres2_numwords == 0))
        {
            s = ARGFETCH_FINALIZE;
        }

        switch(s)
        {
        case ARGFETCH_READING1:
        case ARGFETCH_READING2:
        {
            MemAddr voffset, totalsize;

            if (s == ARGFETCH_READING1)
            {
                voffset = req.argres1_base_address + m_currentArgumentOffset;
                totalsize = req.argres1_numwords * 4;
            }
            else
            {
                voffset = req.argres2_base_address + m_currentArgumentOffset;
                totalsize = req.argres2_numwords * 4;
            }

            // transfer size:
            // - cannot be greater than the line size
            // - cannot be greated than the number of bytes remaining on the ROM
            // - cannot cause the range [voffset + size] to cross over a line boundary.
            MemSize transfer_size = std::min(std::min((MemSize)(totalsize - m_currentArgumentOffset), (MemSize)m_lineSize), 
                                             (MemSize)(m_lineSize - voffset % m_lineSize));

            if (!m_iobus.SendReadRequest(m_devid, req.dca_device_id, voffset, transfer_size))
            {
                DeadlockWrite("Unable to send DCA read request for %#016llx/%u to device %u", (unsigned long long)voffset, (unsigned)transfer_size, (unsigned)req.dca_device_id);
                return FAILED;
            }

            COMMIT {
                ++m_numPendingDCAReads;
                if (m_currentArgumentOffset + transfer_size < totalsize) 
                {
                    m_currentArgumentOffset += transfer_size;
                }
                else if (s == ARGFETCH_READING1)
                {
                    m_currentArgumentOffset = 0;
                    m_fetchState = ARGFETCH_READING2;
                }
                else
                {
                    m_fetchState = ARGFETCH_FINALIZE;
                    // wait until the last read completion re-activates
                    // this process.
                    p_argumentFetch.Deactivate();
                }
            }
            return SUCCESS;
        }
        case ARGFETCH_FINALIZE:
        {
            ProcessRequest preq;

            COMMIT {
                preq.procedure_id = req.procedure_id;
                preq.dca_device_id = req.dca_device_id;
                preq.extra_arg1 = req.extra_arg1;
                preq.extra_arg2 = req.extra_arg2;
                preq.argres1_base_address = req.argres1_base_address;
                preq.argres2_base_address = req.argres2_base_address;
                preq.notification_channel_id = req.notification_channel_id;
                preq.completion_tag = req.completion_tag;
                preq.data1.insert(preq.data1.begin(), m_currentArgData1.begin(), m_currentArgData1.begin() + req.argres1_numwords);
                preq.data2.insert(preq.data2.begin(), m_currentArgData2.begin(), m_currentArgData2.begin() + req.argres2_numwords);
            }

            if (!m_ready.Push(preq))
            {
                DeadlockWrite("Unable to push the current request to the ready queue");
                return FAILED;
            }
        
            COMMIT {
                m_fetchState = ARGFETCH_READING1;
                m_currentArgumentOffset = 0;
                m_numPendingDCAReads = 0;
            }

            m_incoming.Pop();
            return SUCCESS;
        }
        }
        // unreachable
        assert(false);
        return SUCCESS;
    }    


    Result RPCInterface::DoProcessRequests()
    {
        assert(!m_ready.Empty());

        const ProcessRequest& req = m_ready.Front();

        DebugIOWrite("Processing RPC request from client %u for procedure %u, completion tag %#016llx",
                     (unsigned)req.dca_device_id, (unsigned)req.procedure_id, (unsigned long long)req.completion_tag);

        ProcessResponse res;

        COMMIT {
            res.dca_device_id = req.dca_device_id;
            res.argres1_base_address = req.argres1_base_address;
            res.argres2_base_address = req.argres2_base_address;
            res.notification_channel_id = req.notification_channel_id;
            res.completion_tag = req.completion_tag;
            
            m_provider.Service(req.procedure_id, 
                               res.data1, m_currentArgData1.size(), 
                               res.data2, m_currentArgData2.size(), 
                               req.data1, 
                               req.data2, 
                               req.extra_arg1,
                               req.extra_arg2);
        }

        if (!m_completed.Push(res))
        {
            DeadlockWrite("Unable to push request completion");
            return FAILED;
        }
        m_ready.Pop();
        return SUCCESS;
    }

    Result RPCInterface::DoWriteResponse()
    {
        assert(!m_ready.Empty());

        const ProcessResponse& res = m_completed.Front();

        ResponseWritebackState s = m_writebackState;
    
        // if we are just receiving a result and there is no data, or
        // the response address is set to 0 (ignore), then shortcut to
        // notification.
        if (s == RESULTWB_WRITING1 && (res.data1.size() == 0 || res.argres1_base_address == 0))
        {
            s = (res.data2.size() == 0 || res.argres2_base_address == 0) ? RESULTWB_FINALIZE : RESULTWB_WRITING2;
        }
        else if (s == RESULTWB_WRITING2 && (res.data2.size() == 0 || res.argres2_base_address == 0))
        {
            s = RESULTWB_BARRIER;
        }

        switch(s)
        {
        case RESULTWB_WRITING1:
        case RESULTWB_WRITING2:
        {

            MemAddr voffset; 
            MemSize totalsize; 
            const uint32_t *data;
            if (s == RESULTWB_WRITING1)
            {
                voffset = res.argres1_base_address + m_currentResponseOffset;
                totalsize = res.data1.size() * 4;
                data = &res.data1[0];
            }
            else
            {
                voffset = res.argres2_base_address + m_currentResponseOffset;
                totalsize = res.data2.size() * 4;
                data = &res.data2[0];
            }
        
            // transfer size:
            // - cannot be greater than the line size
            // - cannot be greated than the number of bytes remaining on the ROM
            // - cannot cause the range [voffset + size] to cross over a line boundary.
            MemSize transfer_size = std::min(std::min((MemSize)(totalsize - m_currentResponseOffset), (MemSize)m_lineSize), 
                                             (MemSize)(m_lineSize - voffset % m_lineSize));

            size_t res_offset = m_currentResponseOffset / 4;
            size_t num_words = transfer_size / 4;

            DebugIOWrite("Sending response data %d for words %zu - %zu", (s == RESULTWB_WRITING1) ? 1 : 2, res_offset, res_offset + num_words - 1);
            
            IOData iodata;
            iodata.size = transfer_size;
            COMMIT {
                for (size_t i = 0; i < num_words; ++i)
                    SerializeRegister(RT_INTEGER, data[res_offset + i], &iodata.data[i * 4], 4);
            }

            if (!m_iobus.SendWriteRequest(m_devid, res.dca_device_id, voffset, iodata))
            {
                DeadlockWrite("Unable to send DCA write request for %#016llx/%u to device %u", (unsigned long long)voffset, (unsigned)transfer_size, (unsigned)res.dca_device_id);
                return FAILED;
            }

            COMMIT {
                if (m_currentResponseOffset + transfer_size < totalsize) 
                {
                    m_currentResponseOffset += transfer_size;
                }
                else if (s == RESULTWB_WRITING1 && (res.data2.size() != 0 && res.argres2_base_address != 0))
                {
                    m_currentResponseOffset = 0;
                    m_writebackState = RESULTWB_WRITING2;
                }
                else
                {
                    m_writebackState = RESULTWB_BARRIER;
                }
            }
            return SUCCESS;
        }
        case RESULTWB_BARRIER:
        {
            if (!m_iobus.SendReadRequest(m_devid, res.dca_device_id, 0, 0))
            {
                DeadlockWrite("Unable to send DCA write barrier to device %u", (unsigned)res.dca_device_id);
                return FAILED;
            }
        
            COMMIT { 
                m_writebackState = RESULTWB_FINALIZE;
                // wait until the last read completion re-activates
                // this process.
                p_argumentFetch.Deactivate();
            }

            return SUCCESS;
        }
        case RESULTWB_FINALIZE:
        {
            CompletionNotificationRequest creq;
            creq.notification_channel_id = res.notification_channel_id;
            creq.completion_tag = res.completion_tag;

            if (!m_notifications.Push(creq))
            {
                    DeadlockWrite("Unable to push the current result to the notification queue");
                    return FAILED;
            }
            
            COMMIT {
                m_writebackState = RESULTWB_WRITING1;
                m_currentResponseOffset = 0;
            }

            m_completed.Pop();
            return SUCCESS;
        }
        }
        // unreachable
        assert(false);
        return SUCCESS;
    }    

    Result RPCInterface::DoSendCompletionNotifications()
    {
        assert(!m_notifications.Empty());

        const CompletionNotificationRequest& req = m_notifications.Front();

        if (!m_iobus.SendNotification(m_devid, req.notification_channel_id, req.completion_tag))
        {
            DeadlockWrite("Unable to send completion notification to channel %u (tag %#016llx)", (unsigned)req.notification_channel_id, (unsigned long long)req.completion_tag);
            return FAILED;
        }

        m_notifications.Pop();
        return SUCCESS;
    }

    bool RPCInterface::OnReadResponseReceived(IODeviceID from, MemAddr address, const IOData& iodata)
    {
        if (iodata.size == 0)
        {
            // this is a write barrier completion, the writeback process is interested.
            assert(m_writebackState == RESULTWB_FINALIZE);
            COMMIT { 
                m_completed.GetClock().ActivateProcess(p_writeResponse);
            }
        }
        else
        {
            // this is a read completion for the argument read.
            assert(!m_incoming.Empty());

            const IncomingRequest& req = m_incoming.Front();
         
            size_t num_words = iodata.size / 4;
            int anum;
            MemAddr vbase;
            uint32_t* vdata;

            if (address > req.argres1_base_address && address < req.argres1_base_address + req.argres1_numwords)
            {
                vbase = req.argres1_base_address;
                vdata = &m_currentArgData1[0];
                anum = 1;
            }
            else
            {
                assert(address > req.argres2_base_address && address < req.argres2_base_address + req.argres2_numwords);
                vbase = req.argres2_base_address;
                vdata = &m_currentArgData2[0];
                anum = 2;
            }

            size_t arg_offset;

            arg_offset = (address - vbase) / 4;

            DebugIOWrite("Received argument data %d for words %zu - %zu", anum, arg_offset, arg_offset + num_words - 1);

            COMMIT {
                for (size_t i = 0; i < num_words; ++i)
                    vdata[arg_offset + i] = UnserializeRegister(RT_INTEGER, &iodata.data[i * 4], 4);
            }
            
            if (m_fetchState == ARGFETCH_FINALIZE && m_numPendingDCAReads == 1)
            {
                DebugSimWrite("Last read completion, waking up reader process");
                COMMIT { m_incoming.GetClock().ActivateProcess(p_argumentFetch); }
            }
            
            COMMIT { --m_numPendingDCAReads; }
            
        }

        return SUCCESS;
    }

    bool RPCInterface::OnWriteRequestReceived(IODeviceID from, MemAddr address, const IOData& data)
    {
        // word size: 32 bit
        // word 0: command status (0: idle, 1: busy/queueing)
        // word 1: low 16 bits: device ID of DCA peer; high 16 bits: notification channel ID for response
        // word 2: procedure ID
        // word 4: low 32 bits of completion tag
        // word 5: high 32 bits of completion tag
        // word 6: number of words in 1st argument data
        // word 7: number of words in 2nd argument data
        // word 8: low 32 bits of arg/res 1 base address
        // word 9: high 32 bits of arg/res 1 base address
        // word 10: low 32 bits of arg/res 2 base address
        // word 11: high 32 bits of arg/res 2 base address
        // word 12: extra arg 1
        // word 13: extra arg 2

        if (address % 4 != 0 || data.size != 4)
        {
            throw exceptf<SimulationException>(*this, "Invalid unaligned RPC write: %#016llx (%u)", (unsigned long long)address, (unsigned)data.size); 
        }

        unsigned word = address / 4;

        if (word == 3 || word > 13)
        {
            throw exceptf<SimulationException>(*this, "Invalid RPC write to word: %u", word);
        }

        Integer value = UnserializeRegister(RT_INTEGER, data.data, data.size);
        COMMIT{
            switch(word)
            {
            case 0: m_queueEnabled.Set(); break;
            case 1: 
                m_inputLatch.dca_device_id = value & 0xffff; 
                m_inputLatch.notification_channel_id = (value >> 16) & 0xffff;
                break;
            case 2: m_inputLatch.procedure_id = value; break;

            case 4: m_inputLatch.completion_tag      = (m_inputLatch.completion_tag       & ~(MemAddr)0xffffffff) |           (value & 0xffffffff); break;
            case 5: m_inputLatch.completion_tag      = (m_inputLatch.completion_tag       &           0xffffffff) | ((Integer)(value & 0xffffffff) << 32); break;

            case 6: m_inputLatch.argres1_numwords = value; break;
            case 7: m_inputLatch.argres2_numwords = value; break;

            case 8: m_inputLatch.argres1_base_address = (m_inputLatch.argres1_base_address & ~(MemAddr)0xffffffff) |           (value & 0xffffffff); break;
            case 9: m_inputLatch.argres1_base_address = (m_inputLatch.argres1_base_address &           0xffffffff) | ((MemAddr)(value & 0xffffffff) << 32); break;
            case 10: m_inputLatch.argres2_base_address = (m_inputLatch.argres2_base_address & ~(MemAddr)0xffffffff) |           (value & 0xffffffff); break;
            case 11: m_inputLatch.argres2_base_address = (m_inputLatch.argres2_base_address &           0xffffffff) | ((MemAddr)(value & 0xffffffff) << 32); break;

            case 12: m_inputLatch.extra_arg1 = value; break;
            case 13: m_inputLatch.extra_arg2 = value; break;

            }
        }

        return true;
    }

    bool RPCInterface::OnReadRequestReceived(IODeviceID from, MemAddr address, MemSize size)
    {
        // word size: 32 bit
        // word 0: command status (0: idle, 1: busy/queueing)
        // word 1: low 16 bits: device ID of DCA peer; high 16 bits: notification channel ID for response
        // word 2: procedure ID

        // word 4: low 32 bits of completion tag
        // word 5: high 32 bits of completion tag
        // word 6: number of words in area 1
        // word 7: number of words in area 2
        // word 8: low 32 bits of area 1 base address
        // word 9: high 32 bits of area 1 base address
        // word 10: low 32 bits of area 2 base address
        // word 11: high 32 bits of area 2 base address
        // word 12: extra arg 1
        // word 13: extra arg 2
        //
        // word 16: arg/res 1 buffer size
        // word 17: arg/res 2 buffer size
        // word 18: current incoming queue size
        // word 19: max incoming queue size
        // word 20: current ready queue size
        // word 21: max ready queue size
        // word 22: current completed queue size
        // word 23: max completed queue size
        // word 24: current notification queue size
        // word 25: max notification queue size

        if (address % 4 != 0 || size != 4)
        {
            throw exceptf<SimulationException>(*this, "Invalid unaligned RPC read: %#016llx (%u)", (unsigned long long)address, (unsigned)size); 
        }

        unsigned word = address / 4;

        if (word == 3 || (word > 13 && word < 16) || word > 25)
        {
            throw exceptf<SimulationException>(*this, "Invalid RPC read from word: %u", word);
        }

        uint32_t value = 0;
        COMMIT{
            switch(word)
            {
            case 0:  value = m_queueEnabled.IsSet(); break; 

            case 1:  value = (m_inputLatch.dca_device_id & 0xffff) | ((m_inputLatch.notification_channel_id & 0xffff) << 16); break;
            case 2:  value = m_inputLatch.procedure_id; break;

            case 4: value = (m_inputLatch.completion_tag             ) & 0xffffffff; break;
            case 5: value = (m_inputLatch.completion_tag        >> 32) & 0xffffffff; break;

            case 6:  value = m_inputLatch.argres1_numwords; break;
            case 7:  value = m_inputLatch.argres2_numwords; break;

            case 8:  value = (m_inputLatch.argres1_base_address      ) & 0xffffffff; break;
            case 9:  value = (m_inputLatch.argres1_base_address >> 32) & 0xffffffff; break;
            case 10:  value = (m_inputLatch.argres2_base_address      ) & 0xffffffff; break;
            case 11:  value = (m_inputLatch.argres2_base_address >> 32) & 0xffffffff; break;

            case 12:  value = m_inputLatch.extra_arg1; break;
            case 13:  value = m_inputLatch.extra_arg2; break;

            case 16: value = m_currentArgData1.size(); break;
            case 17: value = m_currentArgData2.size(); break;
            case 18: value = m_incoming.size(); break;
            case 19: value = m_incoming.GetMaxSize(); break;
            case 20: value = m_completed.size(); break;
            case 21: value = m_ready.GetMaxSize(); break;
            case 22: value = m_completed.size(); break;
            case 23: value = m_completed.GetMaxSize(); break;
            case 24: value = m_notifications.size(); break;
            case 25: value = m_notifications.GetMaxSize(); break;
            }
        }

        IOData iodata;
        SerializeRegister(RT_INTEGER, value, iodata.data, 4);
        iodata.size = 4;
        
        if (!m_iobus.SendReadResponse(m_devid, from, address, iodata))
        {
            DeadlockWrite("Cannot send RPC read response to I/O bus");
            return false;
        }
        return true;
    }


    void RPCInterface::GetDeviceIdentity(IODeviceIdentification& id) const
    {
        if (!DeviceDatabase::GetDatabase().FindDeviceByName("MGSim", "RPC", id))
        {
            throw InvalidArgumentException(*this, "Device identity not registered");
        }    
    }

    

    

}