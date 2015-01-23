// Copyright 2014 Citra Emulator Project / PPSSPP Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <list>
#include <map>
#include <vector>

#include "common/common.h"
#include "common/thread_queue_list.h"

#include "core/arm/arm_interface.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/hle.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/scheduler.h"
#include "core/hle/result.h"
#include "core/mem_map.h"

namespace Kernel {

void Thread::SetWaitSynchronizationResult(ResultCode result) {
    context.cpu_registers[0] = result.raw;
}

void Thread::SetWaitSynchronizationOutput(s32 output) {
    context.cpu_registers[1] = output;
}

bool Thread::ShouldWait() {
    return status != THREADSTATUS_DORMANT;
}

void Thread::Acquire() {
    _assert_msg_(Kernel, !ShouldWait(), "object unavailable!");
}


// TODO(peachum): this should probably go onto ARM_Interface
/// Resets a thread
static void ResetThread(Thread* t, u32 arg) {
    memset(&t->context, 0, sizeof(Core::ThreadContext));

    t->context.cpu_registers[0] = arg;
    t->context.pc = t->context.reg_15 = t->entry_point;
    t->context.sp = t->stack_top;
    t->context.cpsr = 0x1F; // Usermode

    // TODO(bunnei): This instructs the CPU core to start the execution as if it is "resuming" a
    // thread. This is somewhat Sky-Eye specific, and should be re-architected in the future to be
    // agnostic of the CPU core.
    t->context.mode = 8;
}

/// Prints the thread queue for debugging purposes
static void DebugThreadQueue() {
    Thread* thread = GetCurrentThread();
    if (!thread) {
        return;
    }
    LOG_DEBUG(Kernel, "0x%02X 0x%08X (current)", thread->current_priority, GetCurrentThread()->GetHandle());
    for (auto& t : thread_list) {
        s32 priority = thread_ready_queue.contains(t.get());
        if (priority != -1) {
            LOG_DEBUG(Kernel, "0x%02X 0x%08X", priority, t->GetHandle());
        }
    }
}

/// Creates a thread instance, but does not schedule it
ResultVal<SharedPtr<Thread>> Thread::Create(std::string name, VAddr entry_point,
        u32 arg, s32 processor_id, VAddr stack_top, u32 stack_size) {
    if (stack_size < 0x200) {
        LOG_ERROR(Kernel, "(name=%s): invalid stack_size=0x%08X", name.c_str(), stack_size);
        // TODO: Verify error
        return ResultCode(ErrorDescription::InvalidSize, ErrorModule::Kernel,
                ErrorSummary::InvalidArgument, ErrorLevel::Permanent);
    }

    if (!Memory::GetPointer(entry_point)) {
        LOG_ERROR(Kernel_SVC, "(name=%s): invalid entry %08x", name.c_str(), entry_point);
        // TODO: Verify error
        return ResultCode(ErrorDescription::InvalidAddress, ErrorModule::Kernel,
                ErrorSummary::InvalidArgument, ErrorLevel::Permanent);
    }

    SharedPtr<Thread> thread(new Thread);

    // TODO(yuriks): Thread requires a handle to be inserted into the various scheduling queues for
    //               the time being. Create a handle here, it will be copied to the handle field in
    //               the object and use by the rest of the code. This should be removed when other
    //               code doesn't rely on the handle anymore.
    ResultVal<Handle> handle = Kernel::g_handle_table.Create(thread);
    if (handle.Failed())
        return handle.Code();

    thread->entry_point = entry_point;
    thread->stack_top = stack_top;
    thread->stack_size = stack_size;
    thread->processor_id = processor_id;
    thread->name = std::move(name);

    ResetThread(thread.get(), arg);

    return MakeResult<SharedPtr<Thread>>(std::move(thread));
}

Handle SetupIdleThread() {
    // We need to pass a few valid values to get around parameter checking in Thread::Create.
    auto thread_res = Thread::Create("idle", Memory::KERNEL_MEMORY_VADDR, THREADPRIO_LOWEST, 0,
            THREADPROCESSORID_0, 0, Kernel::DEFAULT_STACK_SIZE);
    _dbg_assert_(Kernel, thread_res.Succeeded());
    SharedPtr<Thread> thread = std::move(*thread_res);

    thread->idle = true;
    CallThread(thread.get());
    return thread->GetHandle();
}

SharedPtr<Thread> SetupMainThread(s32 priority, u32 stack_size) {
    // Initialize new "main" thread
    auto thread_res = Thread::Create("main", Core::g_app_core->GetPC(), priority, 0,
            THREADPROCESSORID_0, Memory::SCRATCHPAD_VADDR_END, stack_size);
    // TODO(yuriks): Propagate error
    _dbg_assert_(Kernel, thread_res.Succeeded());
    SharedPtr<Thread> thread = std::move(*thread_res);

    // If running another thread already, set it to "ready" state
    Thread* cur = GetCurrentThread();
    if (cur && cur->IsRunning()) {
        ChangeReadyState(cur, true);
    }

    // Run new "main" thread
    current_thread = thread.get();
    thread->status = THREADSTATUS_RUNNING;
    Core::g_app_core->LoadContext(thread->context);

    return thread;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ThreadingInit() {
    next_thread_id = INITIAL_THREAD_ID;
    ThreadWakeupEventType = CoreTiming::RegisterEvent("ThreadWakeupCallback", ThreadWakeupCallback);
}

void ThreadingShutdown() {
}

} // namespace
