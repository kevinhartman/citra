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
    virtual void WaitCurrentThread(WaitType wait_type, Object* wait_object) = 0;
    virtual void WaitCurrentThread(WaitType wait_type, Object* wait_object, VAddr wait_address) = 0;
    virtual void ResumeFromWait(Thread* thread) = 0;
    virtual void Sleep(s64 nanoseconds) = 0;
    virtual void SetPriority(Thread* thread, s32 priority) = 0;
    virtual void ExitCurrentThread() = 0;

    virtual Thread* ArbitrateHighestPriorityThread(Object* arbiter, u32 address) = 0;
    virtual void ArbitrateAllThreads(Object* arbiter, u32 address) = 0;
};

};
