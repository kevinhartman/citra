// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "emu_window.h"

void EmuWindow::KeyPressed(KeyMap::HostDeviceKey key) {
    HID::Pad::PadState mapped_key = KeyMap::GetPadKey(key);

    HID::Pad::PadButtonPress(mapped_key);
}

void EmuWindow::KeyReleased(KeyMap::HostDeviceKey key) {
    HID::Pad::PadState mapped_key = KeyMap::GetPadKey(key);

    HID::Pad::PadButtonRelease(mapped_key);
}
