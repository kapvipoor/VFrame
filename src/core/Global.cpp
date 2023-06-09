#include "Global.h"

std::filesystem::path g_DefaultPath	= "D:/Projects/MyPersonalProjects/VFrame/default";
std::filesystem::path g_EnginePath	= "D:/Projects/MyPersonalProjects/VFrame";
std::filesystem::path g_AssetPath	= "D:/Projects/MyPersonalProjects/assets";

bool GetFileExtention(const std::string fileName, std::string& pExtentionn)
{
	// find the last occurance of "."
	std::size_t found = fileName.find_last_of(".");

	// if doesnt exist
	if (found == std::string::npos)
	{
		return false;
	}

	// return estention
	pExtentionn = fileName.substr(found + 1);

	return true;
}

bool GetFileName(const std::string fileName, std::string& pFilename)
{
	// find the last occurance of "."
	std::size_t found = fileName.find_last_of("/");

	// if doesnt exist
	if (found == std::string::npos)
	{
		return false;
	}

	std::string filenameWithExt = fileName.substr(found + 1);
	found = filenameWithExt.find_last_of(".");
	if (found == std::string::npos)
	{
		return false;
	}

	// return name
	pFilename = filenameWithExt.substr(0, found);

	return true;
}
