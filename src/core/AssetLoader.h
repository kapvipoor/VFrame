#pragma once

#include <iostream>
#include <vector>
#include <cmath>

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
	nm::float3					color;
	float						metallic;
	nm::float3					pbr_color;
	float						roughness;
	uint32_t					color_id;
	uint32_t					normal_id;
	uint32_t					roughMetal_id;
	uint32_t					unassigned_0;
	Material() 
		: color_id(MAX_SUPPORTED_TEXTURES)
		, color(0.0f)
		, normal_id(MAX_SUPPORTED_TEXTURES)
		, roughMetal_id(MAX_SUPPORTED_TEXTURES)
		, pbr_color(1.0f)
		, metallic(0.5f)
		, roughness(0.5f)
	{}
};

struct Vertex
{
	nm::float3					pos;
	nm::float3					normal;
	nm::float2					uv;
	nm::float4					tangent;

	Vertex()
	{
		pos = nm::float3(0.0f, 0.0f, 0.0f);
		normal = nm::float3(0.0f, 0.0f, 0.0f);
		uv = nm::float2(0.0f, 0.0f);
		tangent = nm::float4(0.0f, 0.0f, 0.0f, 0.0f);
	}

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

struct BSphere;

struct BVolume
{
	enum Type
	{
			Box					= 0
		,	Sphere				= 1
	};
};

struct BBox : BVolume
{
	enum Type
	{
			Null				= 0
		,	Unit				= 1
		,	Custom				= 2
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
	BBox(Type p_type = Null, Origin p_origin = Auto, nm::float3 p_min = nm::float3(), nm::float3 p_max = nm::float3());
	
	BBox operator* (nm::float4x4 const& p_transform);
	BBox operator* (nm::Transform const& p_transform);
	bool operator==(const BBox& other);
	
	void Merge(const BBox&);
	void Merge(const BSphere&);
	void Reset(Type p_type = Null, Origin p_origin = Auto, nm::float3 p_min = nm::float3(), nm::float3 p_max = nm::float3());

	float GetDepth() const;		// along Z
	float GetHeight() const;	// along Y
	float GetWidth() const;		// along X

	bool isVisiable(BBox);
	bool isVisiable(BSphere);

private:
	void CalculateCorners();
};

struct BSphere : BVolume
{
	nm::float3					bsCenter;
	float						bsRadius;
	BSphere();

	BSphere operator* (nm::Transform const& p_transform);
	bool operator==(const BSphere& other);

	bool isVisiable(BBox);
	bool isVisiable(BSphere);

	BBox GetBbox() { return bsBox; }

private:
	BBox						bsBox;
};

struct SubMesh
{
	std::string					name;
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

struct RawSphere
{
	std::vector<nm::float3> vertices;
	std::vector<int> indices;
	std::vector<int> lineIndices;
};
void GenerateSphere(int p_stackCount, int p_sectorCount, RawSphere& p_sphere, float p_radius = 1.0f);

bool LoadRawImage(const char* p_path, ImageRaw& p_data);
void FreeRawImage(ImageRaw& p_data);

bool LoadGltf(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData);
bool LoadObj(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData);

bool WriteToDisk(const std::filesystem::path& pPath, size_t pDataSize, char* pData);
