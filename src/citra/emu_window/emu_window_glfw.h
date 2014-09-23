// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#pragma once

#include <GLFW/glfw3.h>

#include "common/emu_window.h"

class EmuWindow_GLFW : public EmuWindow {
public:
    EmuWindow_GLFW();
    ~EmuWindow_GLFW();

    /// Swap buffers to display the next frame
    void SwapBuffers();

	/// Polls window events
	void PollEvents();

    /// Makes the graphics context current for the caller thread
    void MakeCurrent();
    
    /// Releases (dunno if this is the "right" word) the GLFW context from the caller thread
    void DoneCurrent();

    /// Gets the size of the window in pixels
    void GetFramebufferSize(int* fbWidth, int* fbHeight);

    static void HandleTouchEvent(double x_position, double y_position);

    static void OnKeyEvent(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void OnMouseButtonEvent(GLFWwindow* win, int button, int action, int mods);
    static void OnMouseLocationUpdate(GLFWwindow* win, double x_position, double y_position);

private:
    GLFWwindow* m_render_window; ///< Internal GLFW render window
    int keyboard_id;             ///< Device id of keyboard for use with KeyMap
    int pixel_ratio;             ///< The ratio of framebuffer pixels to window units
};
