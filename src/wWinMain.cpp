#include <sstream>

#include "core/Global.h"
#include "RasterRender.h"

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    init_console();

    // refer https://stackoverflow.com/questions/5419356/is-there-a-way-to-redirect-stdout-stderr-to-a-string
    std::stringstream cerrBuff;
    std::streambuf* cerrStream = std::cerr.rdbuf(cerrBuff.rdbuf());

    CLOG("Default Path - " << g_DefaultPath << std::endl);
    CLOG("Engine Path - " << g_EnginePath << std::endl);
    CLOG("Asset Path - " << g_AssetPath << std::endl);

    unsigned int val = 1;
    bool exitState = true;
    switch (val)
    {
    case 1:
        {
            CRasterRender rasterRender("VFrame Renderer", DISPLAY_RESOLUTION_X, DISPLAY_RESOLUTION_Y, 1);
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
        MessageBoxA(NULL, cerrBuff.str().c_str(), "Error(s) found!", 0);

    // resetting to original stream - not needed but just following clean code.
    // Ideally this stream redirection should be managed and reset
    // in a utility class but will leave it bare-bones for now
    std::cout.rdbuf(cerrStream);
    
    return EXIT_SUCCESS;
}