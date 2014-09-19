// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "hid_touch.h"
#include "core/hle/hle.h"
#include "core/hle/service/hid.h"

namespace HID {

namespace Touch {

// Next Touch update information
static u32 next_index = 0;

void TouchLocationUpdated(float x, float y) {
    TouchData* touch_data = HID_User::GetTouchData();

    touch_data->index = next_index;
    next_index = (next_index + 1) % touch_data->entries.size();

    // Update current raw touch data
    touch_data->raw_touch_data.x_coord = (u16)x; // TODO: should this be converted to f16?
    touch_data->raw_touch_data.y_coord = (u16)y;
    touch_data->raw_touch_data.SetValid(true);

    // Update touch entry
    TouchDataEntry* current_touch_entry = &touch_data->entries[touch_data->index];
    current_touch_entry->x_coord = (u16)x;
    current_touch_entry->x_coord = (u16)y;
    current_touch_entry->SetValid(true);

    if (touch_data->index == 0) {
        touch_data->index_reset_ticks_previous = touch_data->index_reset_ticks;
        touch_data->index_reset_ticks = (s64)Core::g_app_core->GetTicks();
    }

    HID_User::SignalHIDEvent(HID_User::HIDEventType::PAD_OR_TOUCH);
}

void TouchReleased() {
    TouchData* touch_data = HID_User::GetTouchData();

    // Clear all entries
    for (int i = 0; i < touch_data->entries.size(); i++) {
        TouchDataEntry* entry = &touch_data->entries[i];

        *entry = {};
    }

    next_index = 0;

    // TODO: unknown if event signal should be here too
}


} // namespace

} // namespace
