#include "RTC.h"

#include <sys/time.h>
#include <ctime>
#include <csignal>
#include <cerrno>
#include <cstring>

using namespace std;

namespace Simulator
{

    /* the following implements a very crude semaphore.

       We want that several RTC clocks can update their time
       when a tick event is received. They could update more
       often but we want to limit syscall usage.

       To do this we maintain a count of the RTC clocks
       that have acknowledged the tickevent in "timerTicked".
       The count is reset to the number of RTC clocks
       every time SIGALRM ticks; each RTC clock's event
       handler then decreases it.
    */

    static volatile unsigned g_clockSemaphore = 0;
    static unsigned g_clockListeners = 0;

    static clock_delay_t g_clockResolution = 0;
    static volatile precise_time_t g_currentTime = 0;

    static void set_time(void)
    {
        struct timeval tv;
        int gtod_status = gettimeofday(&tv, NULL);
        assert(gtod_status == 0);
        precise_time_t newTime = tv.tv_usec + tv.tv_sec * 1000000;
        g_currentTime = newTime;
    }

    static void alarm_handler(int)
    {
        g_clockSemaphore = g_clockListeners;
        set_time();
    }

    static void setup_clocks(clock_delay_t d)
    {
        static bool initialized = false;
        if (!initialized)
        {
            g_clockResolution = d;

            set_time(); // need to have a non-zero value before the RTC starts

            if (SIG_ERR == signal(SIGALRM, alarm_handler))
            {
                throw exceptf<>("Cannot set alarm: %s", strerror(errno));
            };

            struct itimerval it;
            it.it_interval.tv_sec = g_clockResolution / 1000000;
            it.it_interval.tv_usec = g_clockResolution % 1000000;
            it.it_value = it.it_interval;

            if (-1 == setitimer(ITIMER_REAL, &it, NULL))
            {
                throw exceptf<>("Cannot set timer: %s", strerror(errno));
            };

            initialized = true;
        }
    }

    const std::string& RTC::RTCInterface::GetIODeviceName() const
    {
        return GetName();
    }

    RTC::RTCInterface::RTCInterface(const std::string& name, RTC& parent,
                                    IOMessageInterface& ioif, IODeviceID devid)
        : Object(name, parent),
          m_devid(devid),
          m_ioif(ioif),
          InitStorage(m_doNotify, ioif.RegisterClient(devid, *this), false),
          InitStateVariable(interruptNumber, 0),
          InitProcess(p_notifyTime, DoNotifyTime)
    {
        m_doNotify.Sensitive(p_notifyTime);
    }

    void RTC::RTCInterface::Initialize()
    {
        p_notifyTime.SetStorageTraces(m_ioif.GetBroadcastTraces(m_devid) * m_doNotify);
    }

    RTC::RTC(const string& name, Object& parent, Clock& rtcclock,
             IOMessageInterface& ioif, IODeviceID devid)
        : Object(name, parent),
          m_timerTicked(false),
          InitStateVariable(timeOfLastInterrupt, 0),
          InitStateVariable(triggerDelay, 0),
          InitStateVariable(deliverAllEvents, true),
          InitStorage(m_enableCheck, rtcclock, false),
          m_businterface("if", *this, ioif, devid),
          InitProcess(p_checkTime, DoCheckTime)
    {

        setup_clocks(GetTopConf("RTCMeatSpaceUpdateInterval", clock_delay_t));
        m_timeOfLastInterrupt = g_currentTime;
        ++g_clockListeners;
        m_enableCheck.Sensitive(p_checkTime);

        p_checkTime.SetStorageTraces(opt(m_businterface.m_doNotify));
    }

    Result RTC::RTCInterface::DoNotifyTime()
    {
        if (!m_ioif.SendInterruptRequest(m_devid, m_interruptNumber))
        {
            DeadlockWrite("Cannot send timer interrupt to I/O bus");
            return FAILED;
        }
        m_doNotify.Clear();
        return SUCCESS;
    }

    Result RTC::DoCheckTime()
    {
        if (!m_timerTicked && (g_clockSemaphore != 0))
        {
            m_timerTicked = true;
            --g_clockSemaphore;
        }

        if (m_timerTicked)
        {
            // The clock is configured to deliver interrupts. Check
            // for this.
            if (m_timeOfLastInterrupt + m_triggerDelay <= g_currentTime)
            {
                // Time for an interrupt.
                m_businterface.m_doNotify.Set();

                COMMIT {
                    if (m_deliverAllEvents)
                    {
                        m_timeOfLastInterrupt += m_triggerDelay;
                    }
                    else
                    {
                        m_timeOfLastInterrupt = g_currentTime;
                    }
                }
            }
            COMMIT {
                m_timerTicked = false;
            }
        }
        return SUCCESS;
    }

    void RTC::RTCInterface::GetDeviceIdentity(IODeviceIdentification& id) const
    {
        if (!DeviceDatabase::GetDatabase().FindDeviceByName("MGSim", "RTC", id))
        {
            throw InvalidArgumentException(*this, "Device identity not registered");
        }
    }


    static uint32_t pack_time(const struct tm* tm, bool otherbits)
    {
        if (!otherbits)
        {
            //       bits 0-5: seconds (0-59)
            //       bits 6-11: minutes (0-59)
            //       bits 12-16: hours (0-23)
            //       bits 17-21: day in month (0-30)
            //       bits 22-25: month in year (0-11)
            //       bits 26-31: year from 1970
            return (uint32_t)tm->tm_sec
                | ((uint32_t)tm->tm_min << 6)
                | ((uint32_t)tm->tm_hour << 12)
                | ((uint32_t)tm->tm_mday << 17)
                | ((uint32_t)tm->tm_mon << 22)
                | ((uint32_t)(tm->tm_year + 70) << 26);
        }
        else
        {
            //       bits 0-3: day of week (sunday = 0)
            //       bits 4-12: day of year (0-365)
            //       bit 13: summer time in effect
            //       bits 14-31: offset from UTC in seconds
            return (uint32_t)tm->tm_wday
                | ((uint32_t)tm->tm_yday << 4)
                | ((uint32_t)!!tm->tm_isdst << 13)
#ifdef HAVE_STRUCT_TM_TM_GMTOFF
                | ((uint32_t)tm->tm_gmtoff << 14)
#endif
                ;
        }
    }

    bool RTC::RTCInterface::OnWriteRequestReceived(IODeviceID /*from*/, MemAddr address, const IOData& data)
    {
        unsigned word = address / 4;

        if (address % 4 != 0 || data.size != 4)
        {
            throw exceptf<>(*this, "Invalid unaligned RTC write: %#016llx (%u)", (unsigned long long)address, (unsigned)data.size);
        }
        if (word == 0 || word > 3)
        {
            throw exceptf<>(*this, "Invalid write to RTC word: %u", word);
        }

        Integer value = UnserializeRegister(RT_INTEGER, data.data, data.size);
        RTC& rtc = GetRTC();

        COMMIT{
            switch(word)
            {
            case 1:
            {
                if (value != 0)
                {
                    rtc.m_timeOfLastInterrupt = g_currentTime;
                    rtc.m_enableCheck.Set();
                }
                else
                {
                    m_doNotify.Clear(); // cancel sending the current interrupt if currently ongoing.
                    rtc.m_enableCheck.Clear();
                }
                rtc.m_triggerDelay = value;
                break;
            }
            case 2:   m_interruptNumber = value; break;
            case 3:   rtc.m_deliverAllEvents = value; break;
            }
        }
        return true;
    }

    StorageTraceSet RTC::RTCInterface::GetWriteRequestTraces() const
    {
        return opt(GetRTC().m_enableCheck);
    }

    StorageTraceSet RTC::RTCInterface::GetReadRequestTraces() const
    {
        return m_ioif.GetRequestTraces(m_devid);
    }

    bool RTC::RTCInterface::OnReadRequestReceived(IODeviceID from, MemAddr address, MemSize size)
    {
        // the clock uses 32-bit control words
        // word 0: resolution (microseconds)
        // word 1: interrupt delay (in microseconds, set to 0 to disable interrupts)
        // word 2: interrupt number to generate
        // word 3: whether to deliver all events
        // word 4: microseconds part of current time since jan 1, 1970
        // word 5: seconds part of current time since jan 1, 1970
        // word 6: packed UTC time:
        // word 7: packed UTC time (more):
        // word 8,9: packed local time (same format as UTC)

        unsigned word = address / 4;
        uint32_t value = 0;

        if (address % 4 != 0 || size != 4)
        {
            throw exceptf<>(*this, "Invalid unaligned RTC read: %#016llx (%u)", (unsigned long long)address, (unsigned)size);
        }
        if (word > 9)
        {
            throw exceptf<>(*this, "Read from invalid RTC word: %u", word);
        }

        RTC& rtc = GetRTC();

        COMMIT{
            switch(word)
            {
            case 0:   value = g_clockResolution; break;
            case 1:   value = rtc.m_triggerDelay; break;
            case 2:   value = m_interruptNumber; break;
            case 3:   value = (int)rtc.m_deliverAllEvents; break;
            case 4:   value = g_currentTime % 1000000; break;
            case 5:   value = g_currentTime / 1000000; break;
            case 6: case 7:
            {
                time_t c = time(0);
                struct tm * tm = gmtime(&c);
                value = pack_time(tm, word - 6);
                break;
            }
            case 8: case 9:
            {
                time_t c = time(0);
                struct tm * tm = localtime(&c);
                value = pack_time(tm, word - 8);
                break;
            }
            }
        }

        IOMessage *msg = m_ioif.CreateReadResponse(m_devid, address, 4);
        COMMIT {
            SerializeRegister(RT_INTEGER, value, msg->read_response.data.data, 4);
        }

        if (!m_ioif.SendMessage(m_devid, from, msg))
        {
            DeadlockWrite("Cannot send RTC read response to I/O bus");
            return false;
        }
        return true;
    }


}
