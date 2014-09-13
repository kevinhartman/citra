// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"
#include "core/hw/hid_pad.h"
#include "core/hw/hid_touch.h"
#include "common/bit_field.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace HID_User

// This service is used for interfacing to physical user controls.
// Uses include game pad controls, touchscreen, accelerometers, gyroscopes, and debug pad.

namespace HID_User {

enum HIDEventType {
    PAD_OR_TOUCH,
    ACCELEROMETER,
    GYROSCOPE,
    DEBUG_PAD
};

HID::Pad::PadData* GetPadData();
HID::Touch::TouchData* GetTouchData();
void SignalHIDEvent(HIDEventType type);

/**
 * HID service interface.
 */
class Interface : public Service::Interface {
public:

    Interface();

    ~Interface();

    /**
     * Gets the string port name used by CTROS for the service
     * @return Port name of service
     */
    std::string GetPortName() const {
        return "hid:USER";
    }

};

} // namespace
