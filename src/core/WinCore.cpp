#include "WinCore.h"
#include "../../external/imgui/backends/imgui_impl_win32.h"
// Needs to be declared outside of the cauldron namespace or it won't resolve properly
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

CWinCore::WindowData CWinCore::s_Window = CWinCore::WindowData{};

CWinCore::CWinCore(const char* name, int screen_width_, int screen_height_, int window_scale)
{
    CWinCore::s_Window.title = name;
    size_t length = strlen(name);
    std::wstring wName(length, L'#');
    mbstowcs(&wName[0], name, length);

    m_title = _wcsdup(wName.c_str());

    CWinCore::s_Window.screenWidth = screen_width_;
    CWinCore::s_Window.screenHeight = screen_height_;
    CWinCore::s_Window.windowWidth = screen_width_ * window_scale;
    CWinCore::s_Window.windowHeight = screen_height_ * window_scale;
    CWinCore::s_Window.swapchainID = 0;
    CWinCore::s_Window.inFocus = false;
}

bool CWinCore::initialize()
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

    //DWORD dwExStyle = WS_EX_OVERLAPPEDWINDOW;
    DWORD dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN; //WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
    RECT client_rect = { 0, 0, CWinCore::s_Window.windowWidth, CWinCore::s_Window.windowHeight };
    AdjustWindowRectEx(&client_rect, dwStyle, FALSE, FALSE);
    int width = client_rect.right - client_rect.left;
    int height = client_rect.bottom - client_rect.top;
    CWinCore::s_Window.handle = CreateWindowEx(NULL, m_classname, m_title, dwStyle, 0, 0, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
    SetProp(CWinCore::s_Window.handle, m_classname, (HANDLE)this);

    if (CWinCore::s_Window.handle == NULL)
    {
        return false;
    }

    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    SetWindowPos(CWinCore::s_Window.handle, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    // Initialise key states
    for (int i = 0; i < 2; ++i)
    {
        m_keystate[i] = new short[256];
        memset(m_keystate[i], 0, sizeof(short) * 256);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(CWinCore::s_Window.handle);

    return true;
}

bool CWinCore::run(HINSTANCE p_instance)
{
    if (!on_create(p_instance))
    {
        shutdown();
        return false;
    }

    ShowWindow(CWinCore::s_Window.handle, SW_SHOW);

    bool active = true;

    auto prev_time = std::chrono::system_clock::now();

    while (active)
    {
        // It might be completely unnecessary to compute delta time, mouse and keyboard interrupts
        // Instead let ImGUI do it

        auto current_time = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsed_time = current_time - prev_time;
        prev_time = current_time;
        float delta = elapsed_time.count();

        wchar_t title[256];
        swprintf(title, 256, L"%s - %llu us", m_title, (uint64_t)(delta * 1000000.0f));
        SetWindowText(CWinCore::s_Window.handle, title);

        // Process Windows messages
        MSG msg;

        while (PeekMessage(&msg, CWinCore::s_Window.handle, 0, 0, PM_REMOVE))
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
                if (ScreenToClient(CWinCore::s_Window.handle, &mouse_pos))
                {
                    int prev_Delta[2] = { mouse_delta[0], mouse_delta[1] };

                    if (mouse_pos.x > CWinCore::s_Window.screenWidth)
                    {
                        mouse_pos.x = CWinCore::s_Window.screenWidth;
                        outofWindow = true;
                    }

                    if (mouse_pos.x < 0)
                    {
                        mouse_pos.x = 0;
                        outofWindow = true;
                    }

                    if (mouse_pos.y > CWinCore::s_Window.screenHeight)
                    {
                        mouse_pos.y = CWinCore::s_Window.screenHeight;
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

        ImGui_ImplWin32_NewFrame();

        // User update
        if (!on_update(delta))
        {
            quit();
        }

        // Present
        CWinCore::s_Window.swapchainID ^= 1;
        InvalidateRect(CWinCore::s_Window.handle, NULL, FALSE);
        on_present();

        // Check if Window closed
        active = !!IsWindow(CWinCore::s_Window.handle);
    }

    on_destroy();
    shutdown();

    return true;
}

void CWinCore::quit()
{
    PostQuitMessage(0);
}

void CWinCore::GetCurrentMousePosition(int& x, int& y)
{
    x = cur_mouse_pos[0]; y = cur_mouse_pos[1];
}

void CWinCore::shutdown()
{
    ImGui_ImplWin32_Shutdown();

    for (int i = 0; i < 2; ++i)
    {
        delete[] m_keystate[i];
    }

    free(m_title);
}

LRESULT CWinCore::window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    CWinCore* wdw = (CWinCore*)GetProp(hwnd, L"Window");

    switch (msg)
    {
    case WM_SETFOCUS:
        CWinCore::s_Window.inFocus = true;
        break;

    case WM_KILLFOCUS:
        CWinCore::s_Window.inFocus = false;
        break;

        //case WM_PAINT:
        //{
        //    //return wdw->on_paint();
        //}
    default:;
    }
    
    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);

    return DefWindowProc(hwnd, msg, wparam, lparam);
}
