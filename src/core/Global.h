#pragma once

#include <string>
#include <queue>

#define RETURN_FALSE_IF_FALSE(v)	\
{ if (!v) {	return false; } }		\

#define MAX_SUPPORTED_MESH_ID	2048
#define MAX_SUPPORTED_TEXTURES	2048

extern std::string g_DefaultPath;
extern std::string g_EnginePath;
extern std::string g_AssetPath;

bool GetFileExtention(const std::string fileName, std::string& pExtentionn);
bool GetFileName(const std::string fileName, std::string& pExtentionn);