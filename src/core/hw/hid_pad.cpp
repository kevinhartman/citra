// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include "hid_pad.h"

#include "core/hle/hle.h"
#include "core/hle/service/hid.h"

namespace HID {

namespace Pad {

    // Next Pad state update information
    static PadState next_state = {{0}};
    static u32 next_index = 0;
    static s16 next_circle_x = 0;
    static s16 next_circle_y = 0;

    /**
     * Circle Pad from keys.
     *
     * This is implemented as "pushed all the way to an edge (max) or centered (0)".
     *
     * Indicate the circle pad is pushed completely to the edge in 1 of 8 directions.
     */
    void UpdateNextCirclePadState() {
        static const s16 max_value = 0x9C;
        next_circle_x = next_state.circle_left ? -max_value : 0x0;
        next_circle_x += next_state.circle_right ? max_value : 0x0;
        next_circle_y = next_state.circle_down ? -max_value : 0x0;
        next_circle_y += next_state.circle_up ? max_value : 0x0;
    }

    /**
     * Sets a Pad state (button or button combo) as pressed.
     */
    void PadButtonPress(PadState pad_state) {
        next_state.hex |= pad_state.hex;
        UpdateNextCirclePadState();
    }

    /**
     * Sets a Pad state (button or button combo) as released.
     */
    void PadButtonRelease(PadState pad_state) {
        next_state.hex &= ~pad_state.hex;
        UpdateNextCirclePadState();
    }

    /**
     * Called after all Pad changes to be included in this update have been made,
     * including both Pad key changes and analog circle Pad changes.
     */
    void PadUpdateComplete() {
        PadData* pad_data = HID_User::GetPadData();

        // Update PadData struct
        pad_data->current_state.hex = next_state.hex;
        pad_data->index = next_index;
        next_index = (next_index + 1) % pad_data->entries.size();

        // Get the previous Pad state
        u32 last_entry_index = (pad_data->index - 1) % pad_data->entries.size();
        PadState old_state = pad_data->entries[last_entry_index].current_state;

        // Compute bitmask with 1s for bits different from the old state
        PadState changed;
        changed.hex = (next_state.hex ^ old_state.hex);

        // Compute what was added
        PadState additions;
        additions.hex = changed.hex & next_state.hex;

        // Compute what was removed
        PadState removals;
        removals.hex = changed.hex & old_state.hex;

        // Get the current Pad entry
        PadDataEntry* current_pad_entry = &pad_data->entries[pad_data->index];

        // Update entry properties
        current_pad_entry->current_state.hex = next_state.hex;
        current_pad_entry->delta_additions.hex = additions.hex;
        current_pad_entry->delta_removals.hex = removals.hex;

        // Set circle Pad
        current_pad_entry->circle_pad_x = next_circle_x;
        current_pad_entry->circle_pad_y = next_circle_y;

        // If we just updated index 0, provide a new timestamp
        if (pad_data->index == 0) {
            pad_data->index_reset_ticks_previous = pad_data->index_reset_ticks;
            pad_data->index_reset_ticks = (s64)Core::g_app_core->GetTicks();
        }

        // Signal both handles when there's an update to Pad or touch
        HID_User::SignalHIDEvent(HID_User::HIDEventType::PAD_OR_TOUCH);
    }

    // TODO(peachum):
    // Add a method for setting analog input from joystick device for the circle Pad.
    //
    // This method should:
    //     * Be called after both PadButton<Press, Release>().
    //     * Be called before PadUpdateComplete()
    //     * Set current PadEntry.circle_pad_<axis> using analog data
    //     * Set PadData.raw_circle_pad_data
    //     * Set PadData.current_state.circle_right = 1 if current PadEntry.circle_pad_x >= 41
    //     * Set PadData.current_state.circle_up = 1 if current PadEntry.circle_pad_y >= 41
    //     * Set PadData.current_state.circle_left = 1 if current PadEntry.circle_pad_x <= -41
    //     * Set PadData.current_state.circle_right = 1 if current PadEntry.circle_pad_y <= -41
    
} // namespace

} // namespace
