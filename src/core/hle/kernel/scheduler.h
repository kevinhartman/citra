// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vector>
#include <array>
#include <unordered_map>

#include "common/common.h"
#include "common/thread_queue_list.h"

#include "thread.h"

namespace Kernel {

enum SchedulingBehavior {
    APPLICATION_CORE = 0,
    SYSTEM_CORE = 1
};

class Scheduler {
public:
    virtual void Init() = 0;
    virtual void RegisterCore(ThreadProcessorId id, ARM_Interface* core, SchedulingBehavior scheduling_behavior) = 0;
    virtual void Shutdown() = 0;
    virtual void Update(/* system ticks */) = 0;
    virtual void SetCurrentCore(ThreadProcessorId id) = 0; // TODO: this is a hack

    virtual Thread* GetCurrentThread() = 0;
    virtual void ScheduleThread(Thread* thread, s32 priority) = 0;
    virtual bool IsScheduled(Thread* thread) = 0;
    virtual void WaitCurrentThread_Sleep() = 0;
    virtual void WaitCurrentThread_ArbitrateAddress(VAddr wait_address) = 0;
    virtual void WaitCurrentThread_WaitSynchronization(std::vector<SharedPtr<WaitObject>> wait_objects,
        bool wait_set_output, bool wait_all) = 0;
    virtual void ReleaseWaitObject(Thread* thread, WaitObject* wait_object) = 0;
    virtual void ResumeFromWait(Thread* thread) = 0;
    virtual void WakeThreadAfterDelay(Thread* thread, s64 nanoseconds) = 0;
    virtual void SetPriority(Thread* thread, s32 priority) = 0;
    virtual void ExitCurrentThread() = 0;

    virtual Thread* ArbitrateHighestPriorityThread(u32 address) = 0;
    virtual void ArbitrateAllThreads(u32 address) = 0;

    /**
     * Release an acquired wait object
     * @param wait_object WaitObject to release
     */
    virtual void ReleaseWaitObject(WaitObject* wait_object) = 0;
};

};
