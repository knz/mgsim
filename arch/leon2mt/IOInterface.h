// -*- c++ -*-
#ifndef IOINTERFACE_H
#define IOINTERFACE_H

#include <sim/kernel.h>
#include <sim/inspect.h>
#include <sim/storage.h>
#include <arch/Memory.h>
#include <arch/leon2mt/IOMatchUnit.h>
#include <arch/leon2mt/IOResponseMultiplexer.h>
#include <arch/leon2mt/IONotificationMultiplexer.h>
#include <arch/leon2mt/IOBusInterface.h>
#include <arch/leon2mt/IODirectCacheAccess.h>

namespace Simulator
{
namespace leon2mt
{

class IOInterface : public Object, public Inspect::Interface<Inspect::Info>
{
public:
    class AsyncIOInterface : public leon2mt::MMIOComponent, public Inspect::Interface<Inspect::Info>
    {
    private:
        MemAddr                 m_baseAddr;
        unsigned                m_devAddrBits;

        IOInterface&  GetInterface() const;
        Object& GetLEON2MTParent() const { return *GetParent()->GetParent(); };
    public:
        AsyncIOInterface(const std::string& name, IOInterface& parent);

        size_t GetSize() const;

        Result Read (MemAddr address, void* data, MemSize size, LFID fid, TID tid, const RegAddr& writeback);
        Result Write(MemAddr address, const void* data, MemSize size, LFID fid, TID tid);

        void Cmd_Info(std::ostream& out, const std::vector<std::string>& args) const;

        // for boot sequence
        unsigned GetDeviceAddressBits() const { return m_devAddrBits; }
        MemAddr GetDeviceBaseAddress(IODeviceID dev) const;
    };

    class PNCInterface : public leon2mt::MMIOComponent, public Inspect::Interface<Inspect::Info>
    {
    private:
        MemAddr                 m_baseAddr;
        IOInterface&  GetInterface() const;
        Object& GetLEON2MTParent() const { return *GetParent()->GetParent(); };

    public:
        PNCInterface(const std::string& name, IOInterface& parent);

        size_t GetSize() const;

        Result Read (MemAddr address, void* data, MemSize size, LFID fid, TID tid, const RegAddr& writeback);
        Result Write(MemAddr address, const void* data, MemSize size, LFID fid, TID tid);

        void Cmd_Info(std::ostream& out, const std::vector<std::string>& args) const;

        // for boot sequence
        MemAddr GetDeviceBaseAddress(IODeviceID dev) const;
    };


private:
    size_t                      m_numDevices;
    size_t                      m_numChannels;

    friend class AsyncIOInterface;
    AsyncIOInterface            m_async_io;

    friend class PNCInterface;
    PNCInterface                m_pnc;

    IOResponseMultiplexer       m_rrmux;
    IONotificationMultiplexer   m_nmux;
    IOBusInterface              m_iobus_if;
    IODirectCacheAccess         m_dca;

    bool Read(IODeviceID dev, MemAddr address, MemSize size, const RegAddr& writeback);
    bool Write(IODeviceID dev, MemAddr address, const IOData& data);
    bool WaitForNotification(IONotificationChannelID dev, const RegAddr& writeback);
    bool ConfigureNotificationChannel(IONotificationChannelID dev, Integer mode);
    Object& GetLEON2MTParent() const { return *GetParent(); };

public:
    IOInterface(const std::string& name, LEON2MT& parent, Clock& clock,
                IOMessageInterface& ioif, IODeviceID devid);
    void ConnectMemory(IMemory* memory);

    leon2mt::MMIOComponent& GetAsyncIOInterface() { return m_async_io; }
    leon2mt::MMIOComponent& GetPNCInterface() { return m_pnc; }

    IOResponseMultiplexer& GetReadResponseMultiplexer() { return m_rrmux; }
    IONotificationMultiplexer& GetNotificationMultiplexer() { return m_nmux; }
    IODirectCacheAccess& GetDirectCacheAccess() { return m_dca; }
    IOBusInterface&      GetIOBusInterface()    { return m_iobus_if; }

    MemAddr GetDeviceBaseAddress(IODeviceID dev) const { return m_async_io.GetDeviceBaseAddress(dev); }

    void Initialize();

    // Debugging
    void Cmd_Info(std::ostream& out, const std::vector<std::string>& args) const;
};

}
}

#endif
