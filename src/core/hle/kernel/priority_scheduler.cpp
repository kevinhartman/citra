// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/core_timing.h"
#include "core/hle/kernel/mutex.h"
#include "core/arm/arm_interface.h"

#include "priority_scheduler.h"

namespace Kernel {

Thread* PriorityScheduler::GetCurrentThread() {
    return cores[current_core_id].current_thread;
}

PriorityScheduler::ThreadState* PriorityScheduler::GetCurrentThreadState() {
    return cores[current_core_id].current_thread_state;
}

void PriorityScheduler::ClampPriority(const Thread* thread, s32* priority) {
    if (*priority < THREADPRIO_HIGHEST || *priority > THREADPRIO_LOWEST) {
        s32 new_priority = CLAMP(*priority, THREADPRIO_HIGHEST, THREADPRIO_LOWEST);
        LOG_WARNING(Kernel_SVC, "(name=%s): invalid priority=%d, clamping to %d",
                    thread->name.c_str(), *priority, new_priority);
        // TODO(bunnei): Clamping to a valid priority is not necessarily correct behavior... Confirm
        // validity of this
        *priority = new_priority;
    }
}

void PriorityScheduler::ScheduleThread(Thread* thread, s32 priority) {
    _dbg_assert_(Kernel, thread);

    ClampPriority(thread, &priority);

    // Thread should not pre-exist in scheduler
    _dbg_assert_(Kernel, thread_data.find(thread) == thread_data.end());

    Core* core = nullptr;
    core = &cores[ThreadProcessorId::THREADPROCESSORID_0];

    // Pick a core that suits this thread
    int core_mask = 1;
    for (auto i : cores) {
        if (~thread->processor_id & core_mask) {
            core = &cores[~core_mask];
            break;
        }
        core_mask = core_mask << 1;
    }

    thread_list.push_back(thread);
    core->thread_ready_queue.prepare(priority);

    ThreadState* state = &thread_data[thread];
    state->thread = thread;
    state->thread_id = NewThreadId();
    state->core = core;
    state->initial_priority = state->current_priority = priority;
    state->status = THREADSTATUS_DORMANT;

    state->wait_objects.clear();
    state->wait_address = 0;

    MakeReady(state);

    // TODO(peachum): switch to the thread regardless of priority immediately
    // to mimic 3DS scheduler behavior on startThread
}

void PriorityScheduler::WaitCurrentThread_Sleep() {
    ThreadState* state = GetCurrentThreadState();
    MakeNotReady(state, THREADSTATUS_WAIT_SLEEP);
    Reschedule(state->core);
}

void PriorityScheduler::WaitCurrentThread_WaitSynchronization(
    std::vector<SharedPtr<WaitObject>> wait_objects, bool wait_set_output, bool wait_all) {

    ThreadState* state = GetCurrentThreadState();
    state->wait_set_output = wait_set_output;
    state->wait_all = wait_all;
    state->wait_objects.insert(state->wait_objects.end(), wait_objects.begin(), wait_objects.end());

    MakeNotReady(state, THREADSTATUS_WAIT_SYNC);
    Reschedule(state->core);
}

void PriorityScheduler::WaitCurrentThread_ArbitrateAddress(VAddr wait_address) {
    ThreadState* state = GetCurrentThreadState();
    state->wait_address = wait_address;
    MakeNotReady(state, THREADSTATUS_WAIT_ARB);
    Reschedule(state->core);
}

/// Resumes a thread from waiting by marking it as "ready"
void PriorityScheduler::ResumeFromWait(Thread* thread) {
    _dbg_assert_(Kernel, thread);
    _dbg_assert_(Kernel, thread_data.find(thread) != thread_data.end());

    // Cancel any outstanding wakeup events for this thread
    CoreTiming::UnscheduleEvent(thread_wakeup_event_id, thread->GetHandle());

    ThreadState* state = &thread_data[thread];

    switch (state->status) {
        case THREADSTATUS_WAIT_SYNC:
            // Remove this thread from all other WaitObjects
            for (auto wait_object : state->wait_objects)
                wait_object->RemoveWaitingThread(thread);
            break;
        case THREADSTATUS_WAIT_ARB:
        case THREADSTATUS_WAIT_SLEEP:
            break;
        case THREADSTATUS_READY:
            LOG_DEBUG(Kernel, "Thread with handle %d has already resumed. Ignoring.", thread->GetHandle());
            break;

        default:
            // This should never happen, as threads must complete before being stopped.
            LOG_ERROR(Kernel, "Thread with handle %d cannot be resumed because it's %d.",
                      thread->GetHandle(), state->status);
            _dbg_assert_(Kernel, false);
            break;
    }

    MakeReady(state);
}

void PriorityScheduler::WakeThreadAfterDelay(Thread* thread, s64 nanoseconds) {
    //_dbg_assert_(Kernel, current_thread);

    // Don't schedule a wakeup if the thread wants to sleep forever
    if (nanoseconds == -1)
        return;

    u64 microseconds = nanoseconds / 1000;
    CoreTiming::ScheduleEvent(usToCycles(microseconds), thread_wakeup_event_id, thread->GetHandle());
}

/// Set the priority of the thread specified by handle
void PriorityScheduler::SetPriority(Thread* thread, s32 priority) {
    _dbg_assert_(Kernel, thread);
    _dbg_assert_(Kernel, thread_data.find(thread) != thread_data.end());

    ClampPriority(thread, &priority);

    ThreadState* state = &thread_data[thread];

    if (state->current_priority == priority) {
        return;
    }

    if (THREADSTATUS_READY == state->status) {
        // If thread was ready, adjust queues
        state->core->thread_ready_queue.remove(state->current_priority, state);
        state->core->thread_ready_queue.prepare(priority);
        state->core->thread_ready_queue.push_back(priority, state);
    }

    state->current_priority = priority;
}

/// Stops the current thread
void PriorityScheduler::ExitCurrentThread() {
    Thread* current_thread = GetCurrentThread();
    ThreadState* state = GetCurrentThreadState();

    // Release all the mutexes that this thread holds
    ReleaseThreadMutexes(current_thread->GetHandle());

    // Cancel any outstanding wakeup events for this thread
    CoreTiming::UnscheduleEvent(thread_wakeup_event_id, current_thread->GetHandle());

    MakeNotReady(state, THREADSTATUS_DEAD);

    current_thread->WakeupAllWaitingThreads();

    Reschedule(state->core);
}

/// Arbitrate the highest priority thread that is waiting
Thread* PriorityScheduler::ArbitrateHighestPriorityThread(u32 address) {
    Thread* highest_priority_thread = nullptr;
    s32 priority = THREADPRIO_LOWEST;

    // TODO(peachum): use a map of threadstate sorted by arb. info to avoid lookup
    // Iterate through threads, find highest priority thread that is waiting to be arbitrated...
    for (auto& thread : thread_list) {
        ThreadState* state = &thread_data[thread.get()];
        if (!CheckWait_AddressArbiter(state, address))
            continue;

        // TODO(peachum): this will never happen if ended threads are removed from thread_list
        if (thread == nullptr)
            continue; // TODO(yuriks): Thread handle will hang around forever. Should clean up.

        if(state->current_priority <= priority) {
            highest_priority_thread = thread.get();
            priority = state->current_priority;
        }
    }

    // If a thread was arbitrated, resume it
    if (nullptr != highest_priority_thread) {
        ResumeFromWait(highest_priority_thread);
    }

    return highest_priority_thread;
}

/// Arbitrate all threads currently waiting
void PriorityScheduler::ArbitrateAllThreads(u32 address) {
    for (auto& thread : thread_list) {
        ThreadState* state = &thread_data[thread.get()];

        if (CheckWait_AddressArbiter(state, address))
            ResumeFromWait(thread.get());
    }
}

/// Changes a threads state
void PriorityScheduler::MakeReady(ThreadState* state) { // TODO(peachum): pass core here?
    Core* core = state->core;

    // TODO(peachum): why is the current thread moved to front instead of back like everyone else?
    if (THREADSTATUS_RUNNING == state->status) {
        core->thread_ready_queue.push_front(state->current_priority, state);
    } else {
        core->thread_ready_queue.push_back(state->current_priority, state);
    }

    state->status = THREADSTATUS_READY;
}

void PriorityScheduler::MakeNotReady(ThreadState* state, ThreadStatus reason) {
    _dbg_assert_(Kernel, (reason & THREADSTATUS_READY) == 0)

    if (THREADSTATUS_READY == state->status) {
        state->core->thread_ready_queue.remove(state->current_priority, state);
    }

    state->status = reason;
}

/// Switches CPU context to that of the specified thread
void PriorityScheduler::SwitchContext(Core* core, Thread* thread) {
    Thread* current_thread = core->current_thread;

    // Save context for current thread
    if (current_thread) {
        core->arm_core->SaveContext(current_thread->context);

        MakeReady(core->current_thread_state);
    }

    // Load context of new thread
    if (thread) {
        ThreadState* state = &thread_data[thread];
        core->current_thread = thread;
        core->current_thread_state = state;

        MakeNotReady(state, THREADSTATUS_RUNNING);
        state->core = core; // TODO(peachum): do we need core on state?

        core->arm_core->LoadContext(thread->context);

    } else {
        core->current_thread = nullptr;
        core->current_thread_state = nullptr;
    }
}

/// Reschedules to the next available thread (call after current thread is suspended)
void PriorityScheduler::Reschedule(PriorityScheduler::Core* core) {

    // TODO(bunnei): It seems that games depend on some CPU execution time elapsing during HLE
    // routines. This simulates that time by artificially advancing the number of CPU "ticks".
    // The value was chosen empirically, it seems to work well enough for everything tested, but
    // is likely not ideal. We should find a more accurate way to simulate timing with HLE.
    core->arm_core->AddTicks(4000);
    core->arm_core->PrepareReschedule();

    Thread* prev = core->current_thread;
    Thread* next = PopNextReadyThread(core);

    if (next != nullptr) {
        LOG_TRACE(Kernel, "context switch 0x%08X -> 0x%08X", prev->GetHandle(), next->GetHandle());
        SwitchContext(core, next);
    } else {
        LOG_TRACE(Kernel, "cannot context switch from 0x%08X, no higher priority thread!", prev->GetHandle());
        // TODO(peachum): context switch to null if need be to have idling

        for (auto& thread : thread_list) {
#ifdef _DEBUG
            ThreadState* state = &thread_data[thread.get()];
#endif
            LOG_TRACE(Kernel, "\thandle=0x%08X prio=0x%02X, status=0x%08X", thread->GetHandle(),
                state->current_priority, state->status);
        }
    }
}

Thread* PriorityScheduler::PopNextReadyThread(Core* core) {
    ThreadState* next;
    ThreadState* state = core->current_thread_state;

    if (state && THREADSTATUS_RUNNING == state->status) {
        next = core->thread_ready_queue.pop_first_better(state->current_priority);
    } else  {
        next = core->thread_ready_queue.pop_first();
    }
    if (next == 0) {
        return nullptr;
    }
    return next->thread;
}

void PriorityScheduler::SetCurrentCore(ThreadProcessorId id) {
    current_core_id = id;
}

void PriorityScheduler::CoreUpdate(ApplicationCore* core/*, system ticks */) {

}

void PriorityScheduler::CoreUpdate(SystemCore* core/*, system ticks */) {

}

void PriorityScheduler::Update(/* system ticks */) {
    for (Core& core : cores) {
        core.Update(this);
    }
}

void PriorityScheduler::RegisterCore(ThreadProcessorId id, ARM_Interface* arm_core, SchedulingBehavior scheduling_behavior) {
    Core* core = nullptr;

    switch (scheduling_behavior) {
        case APPLICATION_CORE:
            core = &(cores[id] = ApplicationCore());
            break;
        case SYSTEM_CORE:
            core = &(cores[id] = SystemCore());
        break;
        default:
            _dbg_assert_(Kernel, false); // Unimplemented scheduling behavior
            core = &(cores[id] = ApplicationCore());
        break;
    }

    core->arm_core = arm_core;
}

void PriorityScheduler::Init() {
    thread_wakeup_event_id = CoreTiming::RegisterEvent("ThreadWakeupCallback", [&](u64 parameter, int cycles_late) {
        this->ThreadWakeupCallback(parameter, cycles_late);
    });
}

void PriorityScheduler::Shutdown() {
}

/// Callback that will wake up the thread it was scheduled for
void PriorityScheduler::ThreadWakeupCallback(u64 parameter, int cycles_late) {
    Handle handle = static_cast<Handle>(parameter);
    SharedPtr<Thread> thread = Kernel::g_handle_table.Get<Thread>(handle);
    if (thread == nullptr) {
        LOG_ERROR(Kernel, "Thread doesn't exist %u", handle);
        return;
    }

    // Set results on thread if a wait syncronization timed out
    ThreadState* state = &thread_data[thread.get()];

    if (THREADSTATUS_WAIT_SYNC == state->status) {
        thread->SetWaitSynchronizationResult(ResultCode(ErrorDescription::Timeout, ErrorModule::OS,
            ErrorSummary::StatusChanged, ErrorLevel::Info));

        if (state->wait_set_output)
            thread->SetWaitSynchronizationOutput(-1);
    }
    
    ResumeFromWait(thread.get());
}

/// Check if a thread is waiting on a the specified wait object
bool PriorityScheduler::CheckWait_WaitObject(const ThreadState* state, WaitObject* wait_object) {
    if (THREADSTATUS_WAIT_SYNC != state->status) return false;

    auto itr = std::find(state->wait_objects.begin(), state->wait_objects.end(), wait_object);
    return itr != state->wait_objects.end();
}

/// Check if the specified thread is waiting on the specified address to be arbitrated
bool PriorityScheduler::CheckWait_AddressArbiter(const ThreadState* state, VAddr wait_address) {
    return THREADSTATUS_WAIT_ARB == state->status && wait_address == state->wait_address;
}
    
}
