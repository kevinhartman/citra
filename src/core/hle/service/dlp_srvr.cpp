// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/hle.h"
#include "core/hle/service/dlp_srvr.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace DLP_SRVR

namespace DLP_SRVR {
    
////////////////////////////////////////////////////////////////////////////////////////////////////
// Interface class

const Interface::FunctionInfo FunctionTable[] = {{}};

Interface::Interface() {
    Register(FunctionTable);
}
    
} // namespace
