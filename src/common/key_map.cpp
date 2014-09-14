// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "key_map.h"
#include <map>

namespace KeyMap {

static std::map<HostDeviceKey, HID::Pad::PadState> key_map;
static int next_device_id = 0;

int NewDeviceId() {
    return next_device_id++;
}

void SetKeyMapping(HostDeviceKey key, HID::Pad::PadState padState) {
    key_map[key].hex = padState.hex;
}

HID::Pad::PadState GetPadKey(HostDeviceKey key) {
    return key_map[key];
}

}
