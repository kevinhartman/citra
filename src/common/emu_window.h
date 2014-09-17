// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include "common/common.h"
#include "common/scm_rev.h"

#include "common/key_map.h"

// Abstraction class used to provide an interface between emulation code and the frontend (e.g. SDL, 
//  QGLWidget, GLFW, etc...)
class EmuWindow
{

public:
    /// Data structure to store an emuwindow configuration
    struct Config{
        bool    fullscreen;
        int     res_width;
        int     res_height;
    };

    /// Swap buffers to display the next frame
    virtual void SwapBuffers() = 0;

    /// Polls window events
    virtual void PollEvents() = 0;

    /// Makes the graphics context current for the caller thread
    virtual void MakeCurrent() = 0;

    /// Releases (dunno if this is the "right" word) the GLFW context from the caller thread
    virtual void DoneCurrent() = 0;

    /// Gets the size of the window in pixels
    virtual void GetFramebufferSize(int* fbWidth, int* fbHeight) = 0;

    /// Signals a key press action to the HID module
    static void KeyPressed(KeyMap::HostDeviceKey key);

    /// Signals a key release action to the HID module
    static void KeyReleased(KeyMap::HostDeviceKey key);

    Config GetConfig() const { 
        return m_config;
    }

    void SetConfig(const Config& val) {
        m_config = val;
    }
    
    int GetClientAreaWidth() const { 
        return m_client_area_width;
    }

    void SetClientAreaWidth(const int val) {
        m_client_area_width = val;
    }

    int GetClientAreaHeight() const {
        return m_client_area_height;
    }

    void SetClientAreaHeight(const int val) {
        m_client_area_height = val;
    }

    std::string GetWindowTitle() const { 
        return m_window_title;
    }
    
    void SetWindowTitle(std::string val) {
        m_window_title = val;
    }

protected:
    EmuWindow() : m_client_area_width(640), m_client_area_height(480) {
        char window_title[255];
        sprintf(window_title, "Citra | %s-%s", Common::g_scm_branch, Common::g_scm_desc);
        m_window_title = window_title;
    }
    virtual ~EmuWindow() {}

    std::string m_window_title;     ///< Current window title, should be used by window impl.

    int m_client_area_width;        ///< Current client width, should be set by window impl.
    int m_client_area_height;       ///< Current client height, should be set by window impl.

private:
    Config m_config;                ///< Internal configuration

};
