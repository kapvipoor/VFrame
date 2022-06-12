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
public:
    int screen_width;
    int screen_height;
    int window_width;
    int window_height;

    virtual ~CWinCore() = default;

    virtual bool on_create(HINSTANCE = NULL) = 0;
    virtual void on_destroy() = 0;
    virtual bool on_update(float delta) = 0;
    virtual void on_present() = 0;

    CWinCore(const char* name, int screen_width_ = 320, int screen_height_ = 240, int window_scale = 2)
    {
        size_t length = strlen(name);
        std::wstring wName(length, L'#');
        mbstowcs(&wName[0], name, length);

        m_title = _wcsdup(wName.c_str());

        screen_width = screen_width_;
        screen_height = screen_height_;
        window_width = screen_width * window_scale;
        window_height = screen_height * window_scale;
    }

    bool initialize()
    {
        // Create window
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = window_proc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = m_classname;

        if (!RegisterClassEx(&wc))
        {
            return false;
        }

        DWORD dwExStyle = WS_EX_OVERLAPPEDWINDOW;
        DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        RECT client_rect = { 0, 0, window_width, window_height };
        AdjustWindowRectEx(&client_rect, dwStyle, FALSE, dwExStyle);
        int width = client_rect.right - client_rect.left;
        int height = client_rect.bottom - client_rect.top;
        m_hwnd = CreateWindowEx(dwExStyle, m_classname, m_title, dwStyle, 0, 0, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
        SetProp(m_hwnd, m_classname, (HANDLE)this);

        if (m_hwnd == NULL)
        {
            return false;
        }

        int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
        SetWindowPos(m_hwnd, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

        // Initialise key states
        for (int i = 0; i < 2; ++i)
        {
            m_keystate[i] = new short[256];
            memset(m_keystate[i], 0, sizeof(short) * 256);
        }

        return true;
    }

    bool run(HINSTANCE p_instance= NULL)
    {
        if (!on_create(p_instance))
        {
            shutdown();
            return false;
        }

        ShowWindow(m_hwnd, SW_SHOW);

        bool active = true;

        auto prev_time = std::chrono::system_clock::now();
        auto lerp_cur_time = std::chrono::system_clock::now();
        auto lerp_end_time = std::chrono::system_clock::now();

        while (active)
        {
            auto current_time = std::chrono::system_clock::now();
            std::chrono::duration<float> elapsed_time = current_time - prev_time;
            prev_time = current_time;
            float delta = elapsed_time.count();

            wchar_t title[256];
            swprintf(title, 256, L"%s - %llu us", m_title, (uint64_t)(delta * 1000000.0f));
            SetWindowText(m_hwnd, title);

            // Process Windows messages
            MSG msg;

            while (PeekMessage(&msg, m_hwnd, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // Update keyboard
            short* current_keystate = m_keystate[m_current_keystate];
            short* prev_keystate = m_keystate[m_current_keystate ^ 1];

            for (int i = 0; i < 256; i++)
            {
                current_keystate[i] = GetAsyncKeyState(i);

                m_keys[i].pressed = false;
                m_keys[i].released = false;

                if (current_keystate[i] != prev_keystate[i])
                {
                    if (current_keystate[i] & 0x8000)
                    {
                        //std::cout << "KeyPressed " << i << std::endl;
                        m_keys[i].pressed = !m_keys[i].down;
                        m_keys[i].down = true;
                    }
                    else
                    {
                        m_keys[i].released = true;
                        m_keys[i].down = false;
                    }
                }
            }

            {
                bool outofWindow = false;
                POINT mouse_pos{};
                if (GetCursorPos(&mouse_pos))
                {
                    if (ScreenToClient(m_hwnd, &mouse_pos))
                    {
                        int prev_Delta[2] = { mouse_delta[0], mouse_delta[1] };

                        if (mouse_pos.x > screen_width)
                        {
                            mouse_pos.x = screen_width;
                            outofWindow = true;
                        }

                        if (mouse_pos.x < 0)
                        {
                            mouse_pos.x = 0;
                            outofWindow = true;
                        }

                        if (mouse_pos.y > screen_height)
                        {
                            mouse_pos.y = screen_height;
                            outofWindow = true;
                        }
                            
                        if (mouse_pos.y < 0)
                        {
                            mouse_pos.y = 0;
                            outofWindow = true;
                        }

                        if (!outofWindow)
                        {
                            mouse_delta[0] = mouse_pos.x - prev_mouse_pos[0];
                            mouse_delta[1] = mouse_pos.y - prev_mouse_pos[1];

                            prev_mouse_pos[0] = mouse_pos.x;
                            prev_mouse_pos[1] = mouse_pos.y;

                            cur_mouse_pos[0] = prev_mouse_pos[0];
                            cur_mouse_pos[1] = prev_mouse_pos[1];

                            //std::cout << "cur_mouse_pos " << cur_mouse_pos[0] << " " << cur_mouse_pos[1] << std::endl;
                        }
                        else
                        {
                            mouse_delta[0] = 0;
                            mouse_delta[1] = 0;
                        }
                    }
                }
            }

            m_current_keystate ^= 1;

            // User update
            if (!on_update(delta))
            {
                quit();
            }

            // Present
            m_swapchainIndex ^= 1;
            InvalidateRect(m_hwnd, NULL, FALSE);
            on_present();

            // Check if Window closed
            active = !!IsWindow(m_hwnd);
        }

        on_destroy();
        shutdown();

        return true;
    }

    void quit() { PostQuitMessage(0); }

  void GetCurrentMousePosition(int& x, int& y) { x = cur_mouse_pos[0]; y = cur_mouse_pos[1];};

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
    void shutdown()
    {
        for (int i = 0; i < 2; ++i)
        {
            delete[] m_keystate[i];
        }

        free(m_title);
    }

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        CWinCore* wdw = (CWinCore*)GetProp(hwnd, L"Window");

        //switch (msg)
        //{
        //    //case WM_PAINT:
        //    //{
        //    //    //return wdw->on_paint();
        //    //}
        //default:;
        //}

        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

    const wchar_t* m_classname = L"Window";

    short* m_keystate[2] = {};
    int m_current_keystate = 0;
    wchar_t* m_title = nullptr;

protected:
    HWND m_hwnd = NULL;
    int m_swapchainIndex = 0;

};