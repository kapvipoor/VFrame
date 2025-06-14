#include "Global.h"
#include <windows.h>

std::ostringstream g_oss = std::ostringstream();
HANDLE g_hConsole;

void init_console()
{
	// display console
	BOOL console = AllocConsole();
	FILE* stdin_console = freopen("CONIN$", "r", stdin);
	FILE* stdout_console = freopen("CONOUT$", "w", stdout);
	FILE* stderr_console = freopen("CONOUT$", "w", stderr);

	g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

void clog_stream(int pMsgColor)
{
	SetConsoleTextAttribute(g_hConsole, pMsgColor);
	std::clog << g_oss.str();
	g_oss.str("");
	g_oss.clear();
	SetConsoleTextAttribute(g_hConsole, 0x0007);
};

void clog_stream()
{
	std::clog << g_oss.str();
	g_oss.str("");
	g_oss.clear();
};

std::filesystem::path g_DefaultPath	= "C:/vkapoor/vframe/default";
std::filesystem::path g_EnginePath	= "C:/vkapoor/vframe";
std::filesystem::path g_AssetPath	= "C:/vkapoor/assets";
