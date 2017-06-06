#include "kernel.h"
#include "storage.h"
#include "sampling.h"
#include <arch/dev/Display.h>

#include <cassert>
#include <cstdarg>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

namespace Simulator
{

#ifdef STATIC_KERNEL
    Kernel* Kernel::g_kernel = 0;
#endif

    void Kernel::Abort()
    {
        m_aborted = true;
    }

    void Kernel::Stop()
    {
        m_suspended = true;
    }

// Returns the Greatest Common Denominator of a and b.
    template<typename T>
    static
    T gcd(T a, T b)
    {
        // Euclid's algorithm
        while (b != 0){
            T t = b;
            b = a % b;
            a = t;
        }
        return a;
    }

// Returns the Lowest Common Multiple of a and b.
    template<typename T>
    static inline
    T lcm(T a, T b)
    {
        return (a * b / gcd(a,b));
    }

    Clock& Kernel::CreateClock(unsigned long frequency)
    {
        // We only allow creating clocks before the simulation starts
        assert(m_cycle == 0);

        // Gotta be at least 1 MHz
        assert(frequency > 0);
        frequency = std::max(1UL, frequency);

        /*
          Calculate new master frequency:
          If we have clocks of frequency [f_1, ..., f_n], we want a frequency
          that allows every cycle time to be an integer multiple of the master
          cycle time (e.g., a 300 MHz and 400 MHz clock would create a master
          clock of 1200 MHz, where the 300 MHz clock ticks once every 4 cycles
          and the 400 MHz clock ticks once every 3 cycles).

          This is simply equalizing fractions. e.g., 1/300 and 1/400 becomes
          4/1200 and 3/1200.

          Also, see if we already have this clock.
        */
        unsigned long master_freq = 1;
        for (auto c : m_clocks)
        {
            if (c->m_frequency == frequency)
            {
                // We already have this clock, no need to calculate anything.
                return *c;
            }

            // Find Least Common Multiplier of master_freq and this clock's frequency.
            master_freq = lcm(master_freq, c->m_frequency);
        }
        master_freq = lcm(master_freq, frequency);

        if (m_master_freq != master_freq)
        {
            // The master frequency changed, update the clock periods
            m_master_freq = master_freq;
            for (auto c : m_clocks)
            {
                assert(m_master_freq % c->m_frequency == 0);
                c->m_period = m_master_freq / c->m_frequency;
            }
        }
        assert(m_master_freq % frequency == 0);

        m_clocks.push_back(new Clock(*this, frequency, m_master_freq / frequency));
        return *m_clocks.back();
    }

    RunState Kernel::Step(CycleNo cycles)
    {
        try
        {
            // Time to simulate until
            const CycleNo endcycle = (cycles == INFINITE_CYCLES) ? cycles : m_cycle + cycles;

//             if (m_cycle == 0)
//             {
//                 // Update any changed storages.
//                 // This is just to effect the initialization writes,
//                 // in order to activate the initial processes.
//                 UpdateStorages();
//             }

            // Advance time to the first clock to run.
            if (m_activeClocks != NULL)
            {
                assert(m_activeClocks->m_cycle >= m_cycle);
                m_cycle = m_activeClocks->m_cycle;
            }

            m_aborted = m_suspended = false;
            bool idle = false;
            while (!m_aborted && (!m_suspended || (m_lastsuspend == m_cycle)) && !idle && (endcycle == INFINITE_CYCLES || m_cycle < endcycle))
            {
                // We start each cycle being idle, and see if we did something this cycle
                idle = true;

                // Process the requested storage updates
                // This can activate or deactivate processes due to changes in storages
                // made by processes run in this cycle.
                if (UpdateStorages())
                {
                    // We've update at least one storage
                    idle = false;
                }

                //
                // Acquire phase
                //
                m_phase = PHASE_ACQUIRE;
                for (Clock* clock = m_activeClocks; clock != NULL && m_cycle == clock->m_cycle; clock = clock->m_next)
                {
                    m_clock = clock;
                    for (Process* process = clock->m_activeProcesses; process != NULL; process = process->m_next)
                    {
                        m_process   = process;

                        // This process begins the cycle
                        // This is a purely administrative function and has no simulation effect.
                        process->OnBeginCycle();

                        // If we fail in the acquire stage, don't bother with the check and commit stages
                        Result result = process->m_delegate();
                        if (result == SUCCESS)
                        {
                            process->m_state = STATE_RUNNING;
                        }
                        else
                        {
                            assert(result == FAILED);
                            process->m_state = STATE_DEADLOCK;
                            ++process->m_stalls;
                        }
                    }
                }

                //
                // Arbitrate phase
                //
                for (Clock* clock = m_activeClocks; clock != NULL && m_cycle == clock->m_cycle; clock = clock->m_next)
                {
                    m_clock = clock;
                    for (Arbitrator* arbitrator = clock->m_activeArbitrators; arbitrator != NULL; arbitrator = arbitrator->GetNext())
                    {
                        arbitrator->OnArbitrate();
                        arbitrator->Deactivate();
                    }
                    clock->m_activeArbitrators = NULL;
                }

                //
                // Commit phase
                //
                for (Clock* clock = m_activeClocks; clock != NULL && m_cycle == clock->m_cycle; clock = clock->m_next)
                {
                    m_clock = clock;
                    for (Process* process = clock->m_activeProcesses; process != NULL; process = process->m_next)
                    {
                        if (process->m_state != STATE_DEADLOCK)
                        {
                            m_process   = process;
                            m_phase     = PHASE_CHECK;

                            Result result = process->m_delegate();
                            if (result == SUCCESS)
                            {
                                // This process is done this cycle.
                                // This is a purely administrative function and has no simulation effect.
                                // We call this before the COMMIT phase, so that if this produces an error,
                                // we can still inspect the state that caused it.
                                process->OnEndCycle();

                                m_phase = PHASE_COMMIT;
                                result = process->m_delegate();

                                // If the CHECK succeeded, the COMMIT cannot fail
                                assert(result == SUCCESS);
                                process->m_state = STATE_RUNNING;

                                // We've done something -- we're not idle
                                idle = false;
                            }
                            else
                            {
                                // If a process has nothing to do (DELAYED) it shouldn't have been
                                // called in the first place.
                                assert(result == FAILED);
                                process->m_state = STATE_DEADLOCK;
                            }
                        }
                    }
                }

                if (idle)
                {
                    // We haven't done anything this cycle. Check if there are clocks scheduled
                    // for cycles in the future. If so, we want to still advance the simulation.
                    for (Clock* clock = m_activeClocks; clock != NULL; clock = clock->m_next)
                    {
                        if (clock->m_cycle > m_cycle)
                        {
                            idle = false;
                            break;
                        }
                    }
                }

                auto dm = DisplayManager::GetManager();
                if (dm) dm->OnCycle(m_cycle);

                if (!idle)
                {
                    // Advance the simulation

                    // Update the clocks
                    for (Clock *next, *clock = m_activeClocks; clock != NULL && m_cycle == clock->m_cycle; clock = next)
                    {
                        next = clock->m_next;

                        // We ran this clock, remove it from the queue
                        m_activeClocks = clock->m_next;
                        clock->m_activated = false;

                        assert(clock->m_activeArbitrators == NULL);

                        if (clock->m_activeProcesses != NULL || clock->m_activeStorages != NULL)
                        {
                            // This clock still has active components, reschedule it
                            ActivateClock(*clock);
                        }
                    }

                    // Advance time to first clock to run
                    if (m_activeClocks != NULL)
                    {
                        assert(m_activeClocks->m_cycle > m_cycle);
                        m_cycle = m_activeClocks->m_cycle;
                    }
                }
            }

            // In case we overshot the end with the last update
            m_cycle = std::min(m_cycle, endcycle);

            if (m_suspended)
            {
                // prevent aborting on the same cycle twice
                // (ie allow try to resume)
                m_lastsuspend = m_cycle;
                return STATE_ABORTED;
            }
            return idle ? STATE_IDLE : STATE_RUNNING;
        }
        catch (SimulationException& e)
        {
            // Add information about what component/state we were executing
            stringstream details;
            details << "While executing process " << m_process->GetName() << endl
                    << "At master cycle " << m_cycle << endl;
            e.AddDetails(details.str());
            throw;
        }
    }

    void Kernel::ActivateClock(Clock& clock)
    {
        if (!clock.m_activated)
        {
            // Calculate new activation time for clock
            clock.m_cycle = (m_cycle / clock.m_period) * clock.m_period + clock.m_period;

            // Insert clock into list based on activation time (earliest in front)
            Clock **before = &m_activeClocks, *after = m_activeClocks;
            while (after != NULL && after->m_cycle < clock.m_cycle)
            {
                before = &after->m_next;
                after  = after->m_next;
            }
            *before = &clock;
            clock.m_next = after;
            clock.m_activated = true;
        }
    }

    bool Kernel::UpdateStorages()
    {
        bool updated = false;
        for (Clock* clock = m_activeClocks; clock != NULL && (m_cycle == clock->m_cycle); clock = clock->m_next)
        {
            for (Storage *s = clock->m_activeStorages; s != NULL; s = s->GetNext())
            {
                s->Update();
                s->Deactivate();
                updated = true;
            }
            clock->m_activeStorages = NULL;
        }
        return updated;
    }


    void Kernel::SetDebugMode(int flags)
    {
        m_debugMode = flags;
    }

    void Kernel::ToggleDebugMode(int flags)
    {
        m_debugMode ^= flags;
    }

    void
    Kernel::RegisterProcess(Process& p)
    {
        m_proc_registry.insert(&p);
    }

    Kernel::Kernel()
        : m_lastsuspend((CycleNo)-1),
          m_cycle(0),
          m_master_freq(0),
          m_clock(NULL),
          m_process(NULL),
          m_clocks(),
          m_activeClocks(NULL),
          m_phase(PHASE_COMMIT),
          m_debugMode(0),
          m_aborted(false),
          m_suspended(false),
          m_config(NULL),
          m_var_registry(),
          m_proc_registry()
    {
        m_var_registry.RegisterVariable(m_cycle, "kernel.cycle", SVC_CUMULATIVE);
        m_var_registry.RegisterVariable(m_phase, "kernel.phase", SVC_STATE);
    }

    Kernel::~Kernel()
    {
        for (auto c : m_clocks)
            delete c;
    }


}
