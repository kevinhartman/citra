// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/common_types.h"

#include "core/mem_map.h"

#include "core/hle/hle.h"
#include "core/hle/kernel/address_arbiter.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/scheduler.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Kernel namespace

namespace Kernel {

class AddressArbiter : public Object {
public:
    std::string GetTypeName() const override { return "Arbiter"; }
    std::string GetName() const override { return name; }

    static const HandleType HANDLE_TYPE = HandleType::AddressArbiter;
    HandleType GetHandleType() const override { return HANDLE_TYPE; }

    std::string name;   ///< Name of address arbiter object (optional)
};

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Arbitrate an address
ResultCode ArbitrateAddress(Handle handle, ArbitrationType type, u32 address, s32 value, u64 nanoseconds) {
    AddressArbiter* object = Kernel::g_handle_table.Get<AddressArbiter>(handle).get();

    if (object == nullptr)
        return InvalidHandle(ErrorModule::Kernel);

    switch (type) {

    // Signal thread(s) waiting for arbitrate address...
    case ArbitrationType::Signal:
        // Negative value means resume all threads
        if (value < 0) {
            Core::scheduler->ArbitrateAllThreads(address);
        } else {
            // Resume first N threads
            for(int i = 0; i < value; i++)
                Core::scheduler->ArbitrateHighestPriorityThread(address);
        }
        break;

    // Wait current thread (acquire the arbiter)...
    case ArbitrationType::WaitIfLessThan:
        if ((s32)Memory::Read32(address) <= value) {
            Core::scheduler->WaitCurrentThread_ArbitrateAddress(address);
        }
        break;
    case ArbitrationType::WaitIfLessThanWithTimeout:
        if ((s32)Memory::Read32(address) <= value) {
            Core::scheduler->WaitCurrentThread_ArbitrateAddress(address);
            Core::scheduler->WakeThreadAfterDelay(Core::scheduler->GetCurrentThread(), nanoseconds);
        }
        break;
    case ArbitrationType::DecrementAndWaitIfLessThan:
    {
        s32 memory_value = Memory::Read32(address) - 1;
        Memory::Write32(address, memory_value);
        if (memory_value <= value) {
            Core::scheduler->WaitCurrentThread_ArbitrateAddress(address);
        }
        break;
    }
    case ArbitrationType::DecrementAndWaitIfLessThanWithTimeout:
    {
        s32 memory_value = Memory::Read32(address) - 1;
        Memory::Write32(address, memory_value);
        if (memory_value <= value) {
            Core::scheduler->WaitCurrentThread_ArbitrateAddress(address);
            Core::scheduler->WakeThreadAfterDelay(Core::scheduler->GetCurrentThread(), nanoseconds);
        }
        break;
    }

    default:
        LOG_ERROR(Kernel, "unknown type=%d", type);
        return ResultCode(ErrorDescription::InvalidEnumValue, ErrorModule::Kernel, ErrorSummary::WrongArgument, ErrorLevel::Usage);
    }
    return RESULT_SUCCESS;
}

/// Create an address arbiter
AddressArbiter* CreateAddressArbiter(Handle& handle, const std::string& name) {
    AddressArbiter* address_arbiter = new AddressArbiter;
    // TOOD(yuriks): Fix error reporting
    handle = Kernel::g_handle_table.Create(address_arbiter).ValueOr(INVALID_HANDLE);
    address_arbiter->name = name;
    return address_arbiter;
}

/// Create an address arbiter
Handle CreateAddressArbiter(const std::string& name) {
    Handle handle;
    CreateAddressArbiter(handle, name);
    return handle;
}

} // namespace Kernel
