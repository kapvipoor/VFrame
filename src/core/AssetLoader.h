#pragma once

#include <iostream>
#include <vector>

#include "external/NiceMath.h"

#include "Global.h"

struct ImageRaw
{
	unsigned char*				raw;
	int							width;
	int							height;
	int							channels;
};

struct Material
{
	uint32_t					color_id;
	uint32_t					normal_id;
};

struct Vertex
{
	nm::float3					pos;
	nm::float3					normal;
	nm::float2					uv;
	nm::float4					tangent;

	bool operator==(const Vertex& other) const
	{
		return (pos == other.pos &&
			normal == other.normal &&
			uv == other.uv);
	}
};
namespace std 
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<float>()(vertex.pos.x()) ^ (hash<float>()(vertex.pos.y()) << 1)) >> 1) ^ (hash<float>()(vertex.pos.z()) << 1);
		}
	};
}

struct BBox
{
	enum Type
	{
			Null				= 0
		,	Unit				= 1
	};

	enum Origin
	{
			Auto				= 0
		,	Center				= 1
		,	CameraStyle			= 2
	};

	nm::float3					bbMin;
	nm::float3					bbMax;
	nm::float3					bBox[8];
	BBox(Type p_type = Null, Origin p_origin = Auto);
};

struct SubMesh
{
	uint32_t					firstIndex;
	uint32_t					indexCount;
	uint32_t					materialId;
};

struct MeshRaw
{
	std::string					name;
	nm::float4x4				transform;
	std::vector<Vertex>			vertexList;
	std::vector<uint32_t>		indicesList;
	std::vector<SubMesh>		submeshes;
	std::vector<BBox>			submeshesBbox;
	BBox						bbox;
};

struct SceneRaw
{
	std::vector<ImageRaw>		textureList;
	std::vector<MeshRaw>		meshList;
	std::vector<Material>		materialsList;
};

struct ObjLoadData
{
	bool						flipUV;
	bool						loadMeshOnly;
};

nm::float4 ComputeTangent(Vertex p_a, Vertex p_b, Vertex p_c);
void ComputeBBox(BBox& p_bbox);

bool LoadRawImage(const char* p_path, ImageRaw& p_data);
void FreeRawImage(ImageRaw& p_data);

bool LoadGltf(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData);
bool LoadObj(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData);