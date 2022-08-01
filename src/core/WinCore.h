#pragma once

//#define _CRT_SECURE_NO_WARNINGS

//#define _CRT_SECURE_NO_WARNINGS

#include <chrono>
#include <Windows.h>
#include <string>
#include <iostream>

#define LEFT_MOUSE_BUTTON       1
#define RIGHT_MOUSE_BUTTON      2
#define MIDDLE_MOUSE_BUTTON     4

class CWinCore
{
private:
    wchar_t* m_title = nullptr;

protected:
    struct WindowData
    {
        const char* title;

        HWND handle;
        bool inFocus;

        int screenWidth;
        int screenHeight;
        int windowWidth;
        int windowHeight;

        int swapchainID;
    };
    static WindowData s_Window;

public:
    virtual ~CWinCore() = default;
    virtual bool on_create(HINSTANCE = NULL) = 0;
    virtual void on_destroy() = 0;
    virtual bool on_update(float delta) = 0;
    virtual void on_present() = 0;

    CWinCore(const char* name, int screen_width_ = 320, int screen_height_ = 240, int window_scale = 2);
    
    bool initialize();
    bool run(HINSTANCE p_instance = NULL);
    void quit();

  void GetCurrentMousePosition(int& x, int& y);

protected:
    struct KeyState
    {
        bool pressed;
        bool released;
        bool down;
    };

    KeyState m_keys[256] = {};
    int prev_mouse_pos[2] = {0,0};
    int mouse_delta[2] = {};
    int cur_mouse_pos[2] = { -1, -1 };

private:
    void shutdown();

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    const wchar_t* m_classname = L"Window";
    short* m_keystate[2] = {};
    int m_current_keystate = 0;
};