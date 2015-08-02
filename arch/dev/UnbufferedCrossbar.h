// -*- c++ -*-
#ifndef UNBUFFERED_CROSSBAR_H
#define UNBUFFERED_CROSSBAR_H

#include <arch/IOBus.h>
#include <sim/kernel.h>
#include <sim/inspect.h>

#include <vector>

namespace Simulator
{
    /*
     * Unbuffered crossbar: an interconnect with no latency and
     * buffering between send and receive.
     *
     * - Devices are numbered from 0 to N with no holes.
     * - requests to invalid devices fault the simulation.
     */
    class UnbufferedCrossbar : public IIOBus, public Object, public Inspect::Interface<Inspect::Info>
    {
        Clock& m_clock;
        std::vector<IIOBusClient*> m_clients;

        void CheckEndPoints(IODeviceID from, IODeviceID to) const;

    public:
        UnbufferedCrossbar(const std::string& name, Object& parent, Clock& clock);

        /* from IIOBus */
        bool RegisterClient(IODeviceID id, IIOBusClient& client);

        bool SendReadRequest(IODeviceID from, IODeviceID to, MemAddr address, MemSize size);
        bool SendReadResponse(IODeviceID from, IODeviceID to, MemAddr address, const IOData& data);
        bool SendWriteRequest(IODeviceID from, IODeviceID to, MemAddr address, const IOData& data);
        bool SendInterruptRequest(IODeviceID from, IONotificationChannelID which);
        bool SendNotification(IODeviceID from, IONotificationChannelID which, Integer tag);
        bool SendActiveMessage(IODeviceID from, IODeviceID to, MemAddr pc, Integer arg);

        StorageTraceSet GetReadRequestTraces(IODeviceID from) const;
        StorageTraceSet GetWriteRequestTraces() const;
        StorageTraceSet GetReadResponseTraces() const;
        StorageTraceSet GetInterruptRequestTraces() const;
        StorageTraceSet GetNotificationTraces() const;
        StorageTraceSet GetActiveMessageTraces() const;

        void Initialize();

        IODeviceID GetLastDeviceID() const;
        IODeviceID GetDeviceIDByName(const std::string& objname) const;
        Object& GetDeviceByName(const std::string& objname) const;
        void GetDeviceIdentity(IODeviceID which, IODeviceIdentification& id) const;
        IODeviceID GetNextAvailableDeviceID() const;

        Clock& GetClock() { return m_clock; }

        /* debug */
        void Cmd_Info(std::ostream& out, const std::vector<std::string>& arguments) const;

    };
}

#endif