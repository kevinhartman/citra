// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/bit_field.h"

namespace HID {

namespace Touch {

/**
 * Structure of a single entry in the TouchData's touch history array.
 */
struct TouchDataEntry {
    u16 x_coord; // TODO: validate this is unsigned
    u16 y_coord;

    u8 is_valid; // indicates if the touch data is valid

    void SetValid(bool isValid) {
        is_valid = isValid ? 1 : 0;
    }

    u8 pad1;
    u16 pad2;
};

/**
 * Structure of all data related to the touch screen.
 */
struct TouchData {
    s64 index_reset_ticks;
    s64 index_reset_ticks_previous;
    u32 index;

    u32 pad1;

    TouchDataEntry raw_touch_data; // Current raw touch coords before conversion to pixels
    
    std::array<TouchDataEntry, 8> entries; // Touch history (since current touch began)
};

} // namespace

} // namespace
