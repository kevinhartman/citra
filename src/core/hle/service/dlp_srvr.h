// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/service.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace DLP_SRVR

namespace DLP_SRVR {

    /// Interface to "dlp:" service
    class Interface : public Service::Interface {
    public:
        Interface();

        std::string GetPortName() const override {
        return "dlp:SRVR";
    }
};

} // namespace
