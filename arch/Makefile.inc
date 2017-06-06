# -*- Makefile -*-
MEMORY_SRC = \
        arch/mem/DDR.cpp \
        arch/mem/DDR.p.h \
        arch/mem/DDR.h
BUILT_SOURCES += arch/mem/DDR.h

if ENABLE_MEM_BANKED
MEMORY_SRC += \
	arch/mem/BankedMemory.cpp \
	arch/mem/BankedMemory.p.h \
	arch/mem/BankedMemory.h
BUILT_SOURCES += arch/mem/BankedMemory.h
endif
if ENABLE_MEM_PARALLEL
MEMORY_SRC += \
	arch/mem/ParallelMemory.cpp \
	arch/mem/ParallelMemory.p.h \
	arch/mem/ParallelMemory.h
BUILT_SOURCES += arch/mem/ParallelMemory.h
endif
if ENABLE_MEM_SERIAL
MEMORY_SRC += \
	arch/mem/SerialMemory.cpp \
	arch/mem/SerialMemory.p.h \
	arch/mem/SerialMemory.h
BUILT_SOURCES += arch/mem/SerialMemory.h
endif
if ENABLE_MEM_DDR
MEMORY_SRC += \
        arch/mem/DDRMemory.cpp \
        arch/mem/DDRMemory.p.h \
	arch/mem/DDRMemory.h
BUILT_SOURCES += arch/mem/DDRMemory.h
endif

DEVICE_SRC = \
	arch/dev/IODeviceDatabase.h \
	arch/dev/IODeviceDatabase.cpp \
	arch/dev/RTC.h \
	arch/dev/RTC.cpp \
	arch/dev/LCD.cpp \
	arch/dev/LCD.h \
        arch/dev/Display.h \
        arch/dev/Display.cpp \
	arch/dev/DisplayDraw.cpp \
	arch/dev/sdl_wrappers.h \
        arch/dev/ELF.h \
        arch/dev/ELFLoader.h \
        arch/dev/ELFLoader.cpp \
        arch/dev/ActiveROM.h \
        arch/dev/ActiveROM.cpp \
	arch/dev/RPC.p.h \
        arch/dev/RPC.h \
        arch/dev/RPC.cpp \
        arch/dev/RPC_unix.h \
        arch/dev/RPC_unix.cpp \
        arch/dev/RPCServiceDatabase.h \
        arch/dev/Selector.h \
        arch/dev/Selector.cpp \
        arch/dev/SMC.h \
        arch/dev/SMC.cpp \
        arch/dev/UART.h \
        arch/dev/UART.cpp
BUILT_SOURCES += arch/dev/RPC.h

PROCESSOR_SRC = \
        arch/drisc/ActionInterface.cpp \
        arch/drisc/ActionInterface.h \
	arch/drisc/Allocator.cpp \
	arch/drisc/Allocator.p.h \
	arch/drisc/Allocator.h \
	arch/drisc/AncillaryRegisterFile.cpp \
	arch/drisc/AncillaryRegisterFile.h \
	arch/drisc/DCache.cpp \
	arch/drisc/DCache.h \
	arch/drisc/DebugChannel.h \
	arch/drisc/DebugChannel.cpp \
	arch/drisc/DecodeStage.cpp \
	arch/drisc/DummyStage.cpp \
	arch/drisc/ExecuteStage.cpp \
	arch/drisc/FamilyTable.cpp \
	arch/drisc/FamilyTable.h \
	arch/drisc/FetchStage.cpp \
        arch/drisc/forward.h \
	arch/drisc/ICache.cpp \
	arch/drisc/ICache.h \
	arch/drisc/IOResponseMultiplexer.p.h \
	arch/drisc/IOResponseMultiplexer.h \
	arch/drisc/IOResponseMultiplexer.cpp \
	arch/drisc/IONotificationMultiplexer.h \
	arch/drisc/IONotificationMultiplexer.cpp \
	arch/drisc/IOBusInterface.p.h \
	arch/drisc/IOBusInterface.h \
	arch/drisc/IOBusInterface.cpp \
	arch/drisc/IOInterface.h \
	arch/drisc/IOInterface.cpp \
	arch/drisc/IOMatchUnit.h \
	arch/drisc/IOMatchUnit.cpp \
        arch/drisc/IODirectCacheAccess.p.h \
	arch/drisc/IODirectCacheAccess.h \
        arch/drisc/IODirectCacheAccess.cpp \
        arch/drisc/ISA.cpp \
	arch/drisc/ISA.mtalpha.h \
	arch/drisc/ISA.mtsparc.h \
	arch/drisc/ISA.mips.h \
        arch/drisc/ISA.or1k.h \
	arch/drisc/MemoryStage.cpp \
	arch/drisc/MMUInterface.cpp \
	arch/drisc/MMUInterface.h \
	arch/drisc/Network.cpp \
	arch/drisc/Network.p.h \
	arch/drisc/Network.h \
	arch/drisc/Pipeline.cpp \
	arch/drisc/Pipeline.h \
	arch/drisc/DRISC.cpp \
	arch/drisc/DRISC.h \
	arch/drisc/RAUnit.cpp \
	arch/drisc/RAUnit.h \
	arch/drisc/ReadStage.cpp \
	arch/drisc/RegisterFile.cpp \
	arch/drisc/RegisterFile.h \
	arch/drisc/ThreadTable.cpp \
	arch/drisc/ThreadTable.h \
	arch/drisc/WritebackStage.cpp \
	arch/drisc/PerfCounters.h \
	arch/drisc/PerfCounters.cpp \
        arch/leon2mt/ActionInterface.cpp \
        arch/leon2mt/ActionInterface.h \
	arch/leon2mt/TMU.cpp \
	arch/leon2mt/TMU.p.h \
	arch/leon2mt/TMU.h \
	arch/leon2mt/AncillaryRegisterFile.cpp \
	arch/leon2mt/AncillaryRegisterFile.h \
	arch/leon2mt/DCache.cpp \
	arch/leon2mt/DCache.h \
	arch/leon2mt/DebugChannel.h \
	arch/leon2mt/DebugChannel.cpp \
	arch/leon2mt/DecodeStage.cpp \
	arch/leon2mt/DummyStage.cpp \
	arch/leon2mt/ExecuteStage.cpp \
	arch/leon2mt/FamilyTable.cpp \
	arch/leon2mt/FamilyTable.h \
	arch/leon2mt/FetchStage.cpp \
        arch/leon2mt/forward.h \
	arch/leon2mt/ICache.cpp \
	arch/leon2mt/ICache.h \
	arch/leon2mt/IOResponseMultiplexer.p.h \
	arch/leon2mt/IOResponseMultiplexer.h \
	arch/leon2mt/IOResponseMultiplexer.cpp \
	arch/leon2mt/IONotificationMultiplexer.h \
	arch/leon2mt/IONotificationMultiplexer.cpp \
	arch/leon2mt/IOBusInterface.p.h \
	arch/leon2mt/IOBusInterface.h \
	arch/leon2mt/IOBusInterface.cpp \
	arch/leon2mt/IOInterface.h \
	arch/leon2mt/IOInterface.cpp \
	arch/leon2mt/IOMatchUnit.h \
	arch/leon2mt/IOMatchUnit.cpp \
        arch/leon2mt/IODirectCacheAccess.p.h \
	arch/leon2mt/IODirectCacheAccess.h \
        arch/leon2mt/IODirectCacheAccess.cpp \
        arch/leon2mt/ISA.cpp \
	arch/leon2mt/ISA.leon2mt.h \
	arch/leon2mt/ISA.leon2mt.h \
	arch/leon2mt/MemoryStage.cpp \
	arch/leon2mt/MMUInterface.cpp \
	arch/leon2mt/MMUInterface.h \
	arch/leon2mt/Network.cpp \
	arch/leon2mt/Network.p.h \
	arch/leon2mt/Network.h \
	arch/leon2mt/Pipeline.cpp \
	arch/leon2mt/Pipeline.h \
	arch/leon2mt/LEON2MT.cpp \
	arch/leon2mt/LEON2MT.h \
	arch/leon2mt/RAUnit.cpp \
	arch/leon2mt/RAUnit.h \
	arch/leon2mt/ReadStage.cpp \
	arch/leon2mt/RegisterFile.cpp \
	arch/leon2mt/RegisterFile.h \
	arch/leon2mt/ThreadTable.cpp \
	arch/leon2mt/ThreadTable.h \
	arch/leon2mt/WritebackStage.cpp \
	arch/leon2mt/PerfCounters.h \
	arch/leon2mt/PerfCounters.cpp
        
BUILT_SOURCES += \
	arch/drisc/Allocator.h \
	arch/drisc/Network.h \
	arch/drisc/IOBusInterface.h \
	arch/drisc/IODirectCacheAccess.h \
	arch/drisc/IOResponseMultiplexer.h \
	arch/drisc/DCache.h \
	arch/leon2mt/TMU.h \
	arch/leon2mt/Network.h \
	arch/leon2mt/IOBusInterface.h \
	arch/leon2mt/IODirectCacheAccess.h \
	arch/leon2mt/IOResponseMultiplexer.h \
	arch/leon2mt/DCache.h

        
COMMON_SRC = \
    arch/area.cpp \
	arch/Archures.h \
        arch/BankSelector.h \
        arch/BankSelector.cpp \
	arch/FPU.cpp \
	arch/FPU.h \
	arch/Memory.h \
	arch/Memory.cpp \
	arch/MGSystem.h \
	arch/MGSystem.cpp \
	arch/simtypes.h \
	arch/simtypes.cpp \
	arch/symtable.h \
	arch/symtable.cpp \
	arch/Interconnect.h \
	arch/Interconnect.hpp \
	arch/IOMessageInterface.cpp \
	arch/IOMessageInterface.h \
	arch/ic/Bus.h \
	arch/ic/Crossbar.h \
	arch/ic/DestinationBuffering.h \
	arch/ic/EndPointArbiter.h \
	arch/ic/EndPointRegistry.h \
	arch/ic/SharedMedium.h \
	arch/ic/SourceBuffering.h \
	arch/ic/WireNet.h \
	arch/VirtualMemory.cpp \
	arch/VirtualMemory.h

if ENABLE_MEM_ZLCDMA
MEMORY_SRC += \
	arch/mem/zlcdma/CDMA.h \
	arch/mem/zlcdma/CDMA.cpp \
	arch/mem/zlcdma/Cache.p.h \
	arch/mem/zlcdma/Cache.h \
	arch/mem/zlcdma/Cache.cpp \
	arch/mem/zlcdma/Directory.h \
	arch/mem/zlcdma/Directory.cpp \
	arch/mem/zlcdma/Node.h \
	arch/mem/zlcdma/Node.cpp \
	arch/mem/zlcdma/RootDirectory.h \
	arch/mem/zlcdma/RootDirectory.cpp
BUILT_SOURCES += arch/mem/zlcdma/Cache.h
endif

if ENABLE_MEM_CDMA
MEMORY_SRC += \
	arch/mem/cdma/CDMA.h \
	arch/mem/cdma/CDMA.cpp \
	arch/mem/cdma/Cache.p.h \
	arch/mem/cdma/Cache.h \
	arch/mem/cdma/Cache.cpp \
	arch/mem/cdma/Directory.h \
	arch/mem/cdma/Directory.cpp \
	arch/mem/cdma/Node.h \
	arch/mem/cdma/Node.cpp \
	arch/mem/cdma/RootDirectory.h \
	arch/mem/cdma/RootDirectory.cpp
BUILT_SOURCES += arch/mem/cdma/Cache.h
endif

EXTRA_DIST += \
	arch/drisc/ISA.mips.cpp \
        arch/drisc/ISA.or1k.cpp \
	arch/drisc/ISA.mtalpha.cpp \
	arch/drisc/ISA.mtsparc.cpp \
        arch/leon2mt/ISA.leon2mt.cpp

ARCH_SOURCES = $(COMMON_SRC) $(PROCESSOR_SRC) $(MEMORY_SRC) $(DEVICE_SRC)
