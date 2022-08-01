//#define _CRT_SECURE_NO_WARNINGS

#include "RasterRender.h"

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // display console
    BOOL console = AllocConsole();
    FILE* stdin_console = freopen("CONIN$", "r", stdin);
    FILE* stdout_console = freopen("CONOUT$", "w", stdout);
    FILE* stderr_console = freopen("CONOUT$", "w", stderr);
    
    unsigned int val = 1;
    bool exitState = true;
    switch (val)
    {
    case 1:
        {
            CRasterRender rasterRender("VFrame Renderer", 1920, 1080, 1);
            CWinCore* winCore = &rasterRender;
            
            if (!winCore->initialize())
                exit(EXIT_FAILURE);
    
            exitState = rasterRender.run(hInstance);
        }
        break;
    default:
        return 0;
    }
    
    if (!exitState)
        MessageBox(NULL, L"Error(s) found!", L"Error(s) found!", 0);
    
    return EXIT_SUCCESS;
}