// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "scheduler.h"

namespace Kernel {

class PriorityScheduler : public Scheduler {

public:
    static Scheduler* Get() {
        static PriorityScheduler scheduler;
        return (Scheduler *)&scheduler; // TODO(peachum): why on earth does this need a cast?
    };

    void Init();
    void RegisterCore(ThreadProcessorId id, ARM_Interface* core, SchedulingBehavior scheduling_behavior);
    void Shutdown();
    void Update(/* system ticks */);
    void SetCurrentCore(ThreadProcessorId id);

    Thread* GetCurrentThread();
    void ScheduleThread(Thread* thread, s32 priority);
    void WaitCurrentThread(WaitType wait_type, Object* wait_object);
    void WaitCurrentThread(WaitType wait_type, Object* wait_object, VAddr wait_address);
    void ResumeFromWait(Thread* thread);
    void Sleep(s64 nanoseconds);
    void SetPriority(Thread* thread, s32 priority);
    void ExitCurrentThread();

    Thread* ArbitrateHighestPriorityThread(Object* arbiter, u32 address);
    void ArbitrateAllThreads(Object* arbiter, u32 address);

protected:
    struct Core;

    struct ThreadState {
        u32 thread_id; // TODO(peachum): should this be on Thread?
        Core* core; // The scheduler core that this thread was last used on

        u32 status;
        s32 initial_priority; // for debugging only
        s32 current_priority;

        WaitType wait_type;
        Object* wait_object;
        VAddr wait_address;

        Thread* thread;

        inline bool IsRunning() const { return (status & THREADSTATUS_RUNNING) != 0; }
        inline bool IsStopped() const { return (status & THREADSTATUS_DORMANT) != 0; }
        inline bool IsReady() const { return (status & THREADSTATUS_READY) != 0; }
        inline bool IsWaiting() const { return (status & THREADSTATUS_WAIT) != 0; }
        inline bool IsSuspended() const { return (status & THREADSTATUS_SUSPEND) != 0; }
    };

    struct Core {
        ARM_Interface* arm_core;

        // Lists only ready thread ids.
        Common::ThreadQueueList<ThreadState*, THREADPRIO_LOWEST+1> thread_ready_queue;

        Thread* current_thread;
        ThreadState* current_thread_state;

        SchedulingBehavior behavior;
    };

    ThreadState* GetCurrentThreadState();
    Thread* PopNextReadyThread(Core* core);
    void MakeReady(ThreadState* state);
    void MakeNotReady(ThreadState* state, ThreadStatus reason);
    void SwitchContext(Core* core, Thread* thread);
    void Reschedule(Core* core);
    void ThreadWakeupCallback(u64 parameter, int cycles_late);

private:
    PriorityScheduler() { }
    PriorityScheduler(PriorityScheduler const&);
    void operator=(PriorityScheduler const&);

    inline static u32 const NewThreadId() {
        static u32 thread_id = 1;
        return thread_id++;
    }

    static bool CheckWaitType(const ThreadState* state, WaitType type);
    static bool CheckWaitType(const ThreadState* state, WaitType type, Object* wait_object);
    static bool CheckWaitType(const ThreadState* state, WaitType type, Object* wait_object, VAddr wait_address);

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
