// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "scheduler.h"

namespace Kernel {

class PriorityScheduler : public Scheduler {

public:
    void Init();
    void RegisterCore(ThreadProcessorId id, ARM_Interface* core, SchedulingBehavior scheduling_behavior);
    void Shutdown();
    void Update(/* system ticks */);
    void SetCurrentCore(ThreadProcessorId id);

    Thread* GetCurrentThread();
    void ScheduleThread(Thread* thread, s32 priority);
    void WaitCurrentThread_WaitSynchronization(std::vector<SharedPtr<WaitObject>> wait_objects,
        bool wait_set_output, bool wait_all);
    void WaitCurrentThread_Sleep();
    void WaitCurrentThread_ArbitrateAddress(VAddr wait_address);
    void ResumeFromWait(Thread* thread);
    void WakeThreadAfterDelay(Thread* thread, s64 nanoseconds);
    void SetPriority(Thread* thread, s32 priority);
    void ExitCurrentThread();

    Thread* ArbitrateHighestPriorityThread(u32 address);
    void ArbitrateAllThreads(u32 address);
    void ReleaseWaitObject(WaitObject* wait_object);

protected:
    struct Core;

    enum ThreadPriority {
        THREADPRIO_HIGHEST      = 0,    ///< Highest thread priority
        THREADPRIO_DEFAULT      = 16,   ///< Default thread priority for userland apps
        THREADPRIO_LOW          = 31,   ///< Low range of thread priority for userland apps
        THREADPRIO_LOWEST       = 63,   ///< Thread priority max checked by svcCreateThread
    };

    enum ThreadStatus {
        THREADSTATUS_RUNNING,
        THREADSTATUS_READY,
        THREADSTATUS_WAIT_ARB,
        THREADSTATUS_WAIT_SLEEP,
        THREADSTATUS_WAIT_SYNC,
        THREADSTATUS_DORMANT,
        THREADSTATUS_DEAD
    };

    struct ThreadState {
        u32 thread_id; // TODO(peachum): should this be on Thread?
        Core* core; // The scheduler core that this thread was last used on

        u32 status;
        s32 initial_priority; // for debugging only
        s32 current_priority;

        VAddr wait_address;
        std::vector<SharedPtr<WaitObject>> wait_objects; ///< Objects that the thread is waiting on
        bool wait_all;          ///< True if the thread is waiting on all objects before
        bool wait_set_output;   ///< True if the output parameter should be set on thread wakeup

        Thread* thread;
    };

    struct Core {
        ARM_Interface* arm_core;

        // Lists only ready thread ids.
        Common::ThreadQueueList<ThreadState*, THREADPRIO_LOWEST+1> thread_ready_queue;

        Thread* current_thread;
        ThreadState* current_thread_state;

        virtual void Update(PriorityScheduler* scheduler);
    };

    struct ApplicationCore : Core {
        void Update(PriorityScheduler* scheduler) {
            scheduler->CoreUpdate(this);
        }
    };

    struct SystemCore : Core {
        u64 ticks_since_slice;
        void Update(PriorityScheduler* scheduler) {
            scheduler->CoreUpdate(this);
        }
    };

    void CoreUpdate(ApplicationCore* core);
    void CoreUpdate(SystemCore* core);

    ThreadState* GetCurrentThreadState();
    Thread* PopNextReadyThread(Core* core);
    void MakeReady(ThreadState* state);
    void MakeNotReady(ThreadState* state, ThreadStatus reason);
    void SwitchContext(Core* core, Thread* thread);
    void Reschedule(Core* core);
    void ThreadWakeupCallback(u64 parameter, int cycles_late);

private:
    PriorityScheduler(PriorityScheduler const&);
    void operator=(PriorityScheduler const&);

    inline static u32 const NewThreadId() {
        static u32 thread_id = 1;
        return thread_id++;
    }

    static void ClampPriority(const Thread* thread, s32* priority);
    static bool CheckWait_WaitObject(const ThreadState* state, WaitObject* wait_object);
    static bool CheckWait_AddressArbiter(const ThreadState* thread, VAddr wait_address);

    // Current core to operate on
    ThreadProcessorId current_core_id; //TODO(peachum): this will probably have to be thread_local when we do multi-threading

    // Cores that threads can be run on
    std::unordered_map<ThreadProcessorId, Core> cores;

    // Lists all thread ids that aren't deleted/etc.
    std::vector<SharedPtr<Thread>> thread_list;

    // Scheduling data for threads managed by this scheduler
    std::unordered_map<Thread*, ThreadState> thread_data;
    
    int thread_wakeup_event_id;
};
    
};
