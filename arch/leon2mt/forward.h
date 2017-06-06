// -*- c++ -*-
#ifndef LEON2MT_FORWARD_H
#define LEON2MT_FORWARD_H

// Forward declarations
class Config;
namespace Simulator
{
    class LEON2MT;
    class IBankSelector;

    namespace counters {};

    namespace leon2mt
    {
        class FamilyTable;
        struct Family;
        class ThreadTable;
        struct Thread;
        class RegisterFile;
        class RAUnit;
        class ICache;
        class DCache;
        class Network;
        class Pipeline;
        class TMU;
        class MMIOComponent;
        class IOMatchUnit;
        class IOBusInterface;
        class IOResponseMultiplexer;
        class IONotificationMultiplexer;
        class IODirectCacheAccess;
        class IOInterface;
        struct RemoteMessage;
        struct LinkMessage;
        struct AllocResponse;

        const std::vector<std::string>& GetDefaultLocalRegisterAliases(RegType type);
// ISA-specific function to map virtual registers to register classes
        unsigned char GetRegisterClass(unsigned char addr, const RegsNo& regs, RegClass* rc, RegType type);
    }
}

#endif
