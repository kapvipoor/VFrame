#include "Global.h"
#include <windows.h>

std::ostringstream g_oss = std::ostringstream();
HANDLE g_hConsole;

std::filesystem::path g_DefaultPath = "";
std::filesystem::path g_EnginePath = "";
std::filesystem::path g_AssetPath = "";

void init_global()
{
	g_EnginePath = std::filesystem::current_path().parent_path();
	g_DefaultPath = g_EnginePath / "default";
	g_AssetPath = g_EnginePath.parent_path() / "assets";

	CLOG("Default Path - " << g_DefaultPath << std::endl);
	CLOG("Engine Path - " << g_EnginePath << std::endl);
	CLOG("Asset Path - " << g_AssetPath << std::endl);
}

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
