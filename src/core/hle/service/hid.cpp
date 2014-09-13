// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "common/log.h"

#include "core/hle/hle.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/hid.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace HID_User

namespace HID_User {

// Handle to shared memory region designated to HID_User service
static Handle shared_mem = 0;

// Event handles
static Handle event_pad_or_touch_1 = 0;
static Handle event_pad_or_touch_2 = 0;
static Handle event_accelerometer = 0;
static Handle event_gyroscope = 0;
static Handle event_debug_pad = 0;

/**
 * Gets a pointer to the PadData structure inside HID shared memory.
 */
HID::Pad::PadData* GetPadData() {
    if (0 == shared_mem) {
        return nullptr;
    }

    return reinterpret_cast<HID::Pad::PadData*>(Kernel::GetSharedMemoryPointer(shared_mem, 0x0));
}

/**
 * Gets a pointer to the TouchData structure inside HID shared memory.
 */
HID::Touch::TouchData* GetTouchData() {
    if (0 == shared_mem) {
        return nullptr;
    }

    return reinterpret_cast<HID::Touch::TouchData*>(Kernel::GetSharedMemoryPointer(shared_mem, 0xA8));
}

void SignalHIDEvent(HIDEventType type) {

    const auto signal_and_log = [type](Handle event) {
        if (!Kernel::SignalEvent(event)) {
            ERROR_LOG(LOG_TYPE::HID, "Signal to HID Event with type ID %d failed.", type);
        }
    };

    switch (type) {
        case HIDEventType::PAD_OR_TOUCH:
            signal_and_log(event_pad_or_touch_1);
            signal_and_log(event_pad_or_touch_2);
            break;

        case HIDEventType::ACCELEROMETER:
            signal_and_log(event_accelerometer);
            break;

        case HIDEventType::GYROSCOPE:
            signal_and_log(event_gyroscope);
            break;

        case HIDEventType::DEBUG_PAD:
            signal_and_log(event_debug_pad);

        default:
            ERROR_LOG(LOG_TYPE::HID, "HID Event with type ID %d is unimplmenented.", type);
            break;
    }
}

/**
 * HID_User::GetIPCHandles service function
 *  Inputs:
 *      None
 *  Outputs:
 *      1 : Result of function, 0 on success, otherwise error code
 *      2 : Unused
 *      3 : Handle to HID_User shared memory
 *      4 : Event signaled by HID_User
 *      5 : Event signaled by HID_User
 *      6 : Event signaled by HID_User
 *      7 : Gyroscope event
 *      8 : Event signaled by HID_User
 */
void GetIPCHandles(Service::Interface* self) {
    u32* cmd_buff = Service::GetCommandBuffer();

    cmd_buff[1] = 0; // No error
    cmd_buff[3] = shared_mem;
    cmd_buff[4] = event_pad_or_touch_1;
    cmd_buff[5] = event_pad_or_touch_2;
    cmd_buff[6] = event_accelerometer;
    cmd_buff[7] = event_gyroscope;
    cmd_buff[8] = event_debug_pad;

    DEBUG_LOG(KERNEL, "called");
}

const Interface::FunctionInfo FunctionTable[] = {
    {0x000A0000, GetIPCHandles, "GetIPCHandles"},
    {0x000B0000, nullptr,       "StartAnalogStickCalibration"},
    {0x000E0000, nullptr,       "GetAnalogStickCalibrateParam"},
    {0x00110000, nullptr,       "EnableAccelerometer"},
    {0x00120000, nullptr,       "DisableAccelerometer"},
    {0x00130000, nullptr,       "EnableGyroscopeLow"},
    {0x00140000, nullptr,       "DisableGyroscopeLow"},
    {0x00150000, nullptr,       "GetGyroscopeLowRawToDpsCoefficient"},
    {0x00160000, nullptr,       "GetGyroscopeLowCalibrateParam"},
    {0x00170000, nullptr,       "GetSoundVolume"},
};


////////////////////////////////////////////////////////////////////////////////////////////////////
// Interface class

Interface::Interface() {
    shared_mem = Kernel::CreateSharedMemory("HID_User:SharedMem"); // Create shared memory object

    // Create event handles
    event_pad_or_touch_1 = Kernel::CreateEvent(RESETTYPE_ONESHOT, "HID_User:EventPadOrTouch1");
    event_pad_or_touch_2 = Kernel::CreateEvent(RESETTYPE_ONESHOT, "HID_User:EventPadOrTouch2");
    event_accelerometer = Kernel::CreateEvent(RESETTYPE_ONESHOT, "HID_User:EventAccelerometer");
    event_gyroscope = Kernel::CreateEvent(RESETTYPE_ONESHOT, "HID_User:EventGyroscope");
    event_debug_pad = Kernel::CreateEvent(RESETTYPE_ONESHOT, "HID_User:EventDebugPad");

    Register(FunctionTable, ARRAY_SIZE(FunctionTable));
}

Interface::~Interface() {
}

} // namespace
