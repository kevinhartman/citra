// Copyright 2014 Citra Emulator Project / PPSSPP Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include <vector>

#include "common/common_types.h"

#include "core/core.h"
#include "core/mem_map.h"

#include "core/hle/kernel/kernel.h"
#include "core/hle/result.h"


enum ThreadProcessorId {
    THREADPROCESSORID_0     = 0xFFFFFFFE,   ///< Enables core appcode
    THREADPROCESSORID_1     = 0xFFFFFFFD,   ///< Enables core syscore
    THREADPROCESSORID_ALL   = 0xFFFFFFFC,   ///< Enables both cores
};

enum ThreadPriority {
    THREADPRIO_HIGHEST      = 0,    ///< Highest thread priority
    THREADPRIO_DEFAULT      = 16,   ///< Default thread priority for userland apps
    THREADPRIO_LOW          = 31,   ///< Low range of thread priority for userland apps
    THREADPRIO_LOWEST       = 63,   ///< Thread priority max checked by svcCreateThread
};

namespace Kernel {

class Thread : public WaitObject {
public:
    static ResultVal<SharedPtr<Thread>> Create(std::string name, VAddr entry_point,
        u32 arg, s32 processor_id, VAddr stack_top, u32 stack_size);

    static ResultVal<SharedPtr<Thread>> CreateIdleThread();

    std::string GetName() const override { return name; }
    std::string GetTypeName() const override { return "Thread"; }

    static const HandleType HANDLE_TYPE = HandleType::Thread;
    HandleType GetHandleType() const override { return HANDLE_TYPE; }

    bool ShouldWait() override;
    void Acquire() override;

    /**
      * Sets the result after the thread awakens (from either WaitSynchronization SVC)
      * @param result Value to set to the returned result
      */
    void SetWaitSynchronizationResult(ResultCode result);

    /**
      * Sets the output parameter value after the thread awakens (from WaitSynchronizationN SVC only)
      * @param output Value to set to the output parameter
      */
    void SetWaitSynchronizationOutput(s32 output);

    Core::ThreadContext context;

    // Supplied by user on construction
    u32 thread_id;
    u32 entry_point;
    u32 stack_top;
    u32 stack_size;
    s32 processor_id;
    bool IsIdle() { return idle; }

    std::string name;

private:
    Thread() = default;

    /// Whether this thread is intended to never actually be executed, i.e. always idle
    bool idle = false;
};

/// Sets up the primary application thread
SharedPtr<Thread> SetupMainThread(s32 priority, u32 stack_size);

/// Initialize threading
void ThreadingInit();

/// Shutdown threading
void ThreadingShutdown();

} // namespace
