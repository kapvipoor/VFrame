#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <map>

#include "external/NiceMath.h"


#include "Global.h"

bool GetFileExtention(const std::string fileName, std::string& pExtentionn);

bool GetFileName(const std::string fileName, std::string& pExtentionn, const char pDelimiter[2] = "/");

struct ImageRaw
{
	std::string					name;
	float*						raw_hdr;	// for HDR images
	unsigned char*				raw;
	int							width;
	int							height;
	int							depthOrArraySize;
	int							channels;
	uint32_t					mipLevels;
	std::string					fileExtn;
	ImageRaw()
		: name("")
		, raw_hdr(nullptr)
		, raw(nullptr)
		, width(-1)
		, height(-1)
		, channels(-1)
		, mipLevels(1)
		, depthOrArraySize(-1){}
};

struct Material
{
	nm::float3					emissive;
	float						metallic;
	nm::float3					pbr_color;
	float						roughness;
	uint32_t					color_id;
	uint32_t					normal_id;
	uint32_t					roughMetal_id;
	uint32_t					emissive_id;
	Material() 
		: color_id(MAX_SUPPORTED_TEXTURES)
		, normal_id(MAX_SUPPORTED_TEXTURES)
		, roughMetal_id(MAX_SUPPORTED_TEXTURES)
		, emissive_id(MAX_SUPPORTED_TEXTURES)
		, pbr_color(1.0f)
		, metallic(0.5f)
		, roughness(0.5f)
		, emissive(0.0f)
	{}
};

struct Vertex
{
public:
	enum AttributeFlag
	{
		  position	= 1
		, normal	= 2
		, uv		= 4
		, tangent	= 8
		, id		= 16
		, transform = 32
		, max		= 32
	};

	Vertex(uint32_t p_size, int p_attributeFlags, std::vector<uint32_t> p_offsetList)
	{
		attributeFlags = p_attributeFlags;
		offsetList = p_offsetList;
		raw.resize(p_size);
	}

	bool operator== (const Vertex& o) const 
	{
		return (raw == o.raw);
	}

	bool AddAttribute(AttributeFlag p_attribute, const float* p_data)
	{	
		if (!(p_attribute & attributeFlags))
		{
			std::cerr << "Trying add an incompatible vertex attribute" << std::endl;
			return false;
		}

		uint32_t localIndex = (uint32_t)std::log2((int)p_attribute);
		uint32_t attributeSize = (localIndex + 1 < offsetList.size()) ? 
			(offsetList[localIndex + 1] - offsetList[localIndex]) : (uint32_t)raw.size() - offsetList[localIndex];

		std::copy(p_data , p_data + attributeSize, &raw[offsetList[localIndex]]);

		return true;
	}

	float* GetAttribute(AttributeFlag p_attribute)
	{
		if (!(p_attribute & attributeFlags))
			return nullptr;

		return &raw[offsetList[(uint32_t)std::log2((int)p_attribute)]];
	}

	const float* GetRaw() const { return raw.data(); }

private:
	int						attributeFlags;
	std::vector<float>		raw;
	std::vector<uint32_t>	offsetList;
};

struct VertexList
{
	VertexList(int p_attributeFlags)
	{
		attributeFlags = p_attributeFlags;
		vertexSize = 0;
		if (p_attributeFlags & Vertex::AttributeFlag::position)
		{
			mapAttributeOffset.insert(std::make_pair(Vertex::AttributeFlag::position, vertexSize));
			attribteOffsetList.push_back(vertexSize);
			vertexSize += 3;
		}
		if (p_attributeFlags & Vertex::AttributeFlag::normal)
		{
			mapAttributeOffset.insert(std::make_pair(Vertex::AttributeFlag::normal, vertexSize));
			attribteOffsetList.push_back(vertexSize);
			vertexSize += 3;
		}
		if (p_attributeFlags & Vertex::AttributeFlag::uv)
		{
			mapAttributeOffset.insert(std::make_pair(Vertex::AttributeFlag::uv, vertexSize));
			attribteOffsetList.push_back(vertexSize);
			vertexSize += 2;
		}
		if (p_attributeFlags & Vertex::AttributeFlag::tangent)
		{
			mapAttributeOffset.insert(std::make_pair(Vertex::AttributeFlag::tangent, vertexSize));
			attribteOffsetList.push_back(vertexSize);
			vertexSize += 4;
		}
		if (p_attributeFlags & Vertex::AttributeFlag::id)
		{
			mapAttributeOffset.insert(std::make_pair(Vertex::AttributeFlag::id, vertexSize));
			attribteOffsetList.push_back(vertexSize);
			vertexSize += 1;
		}
		if (p_attributeFlags & Vertex::AttributeFlag::transform)
		{
			mapAttributeOffset.insert(std::make_pair(Vertex::AttributeFlag::transform, vertexSize));
			attribteOffsetList.push_back(vertexSize);
			vertexSize += 16;
		}
	}

	Vertex CreateVertex()
	{
		return Vertex(vertexSize, attributeFlags, attribteOffsetList);
	}

	void AddVertex(const Vertex& vertex) 
	{
		std::copy( vertex.GetRaw(), vertex.GetRaw() + vertexSize, std::back_inserter(raw));
	}

	void AddVertex(Vertex::AttributeFlag attributeFlag, const float* p_data)
	{
		Vertex vertex = CreateVertex();
		if (!vertex.AddAttribute(attributeFlag, p_data))
		{
			std::cerr << "Vertex not added to the list" << std::endl;
		}

		AddVertex(vertex);
	}

	std::vector<float>& getRaw() { return raw; }
	size_t size() const { return raw.size(); }
	const float* data() const { return raw.data(); }

	// Returns the size in float and NOT bytes.
	// TODO: Fix this patch of code or at-least make it
	// more intuitive.
	uint32_t GetVertexSize() const { return vertexSize; }

	uint32_t GetOffsetOf(Vertex::AttributeFlag p_attributeFlag) 
	{
		const auto& it = mapAttributeOffset.find(p_attributeFlag);
		if (it != mapAttributeOffset.end())
		{
			return it->second;
		}
	}

private:
	int											attributeFlags;
	uint32_t									vertexSize;
	std::map<Vertex::AttributeFlag, uint32_t>	mapAttributeOffset;
	std::vector<uint32_t>						attribteOffsetList;
	std::vector<float>							raw;
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			float* raw = const_cast<float*>(vertex.GetRaw());
			if (raw)
			{
				//return ((hash<float>()(vertex.position.x()) ^ (hash<float>()(vertex.position.y()) << 1)) >> 1) ^ (hash<float>()(vertex.position.z()) << 1);
				return ((hash<float>()(raw[0]) ^ (hash<float>()(raw[1]) << 1)) >> 1) ^ (hash<float>()(raw[2]) << 1);
			}

			return 0;
		}
	};
}

struct BSphere;

struct BVolume
{
public:
	enum BType
	{
			Box					= 0
		,	Sphere				= 1
		,	Frustum				= 2
	};

	BVolume(BType p_bType) { bType = p_bType; }
	virtual ~BVolume() {};

	// for polymorphism. Does nothing part from supporting dynamic casting
	virtual void Pure() = 0; 

	BType GetBoundingType() { return bType; }
protected:
	BType bType;
};

struct BFrustum : BVolume
{
	static std::vector<int> GetIndexTemplate();
	static VertexList GetVertexTempate();

	BFrustum(nm::float4x4 p_viewProj);
	~BFrustum() {};

	void Pure() override {};

	//BFrustum operator* (nm::float4x4 const& p_transform);

	nm::float4x4 GetViewProjection() { return viewProjection; }

private:
	nm::float4x4 viewProjection;
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

	static std::vector<uint32_t> GetIndexTemplate();
	static VertexList GetVertexTempate();

	Origin						origin;
	nm::float3					bbMin;
	nm::float3					bbMax;
	nm::Transform				unitBBoxTransform;

	BBox(Type p_type = Null, Origin p_origin = Auto, nm::float3 p_min = nm::float3(), nm::float3 p_max = nm::float3());

	void Pure() override {};

	BBox operator* (nm::Transform const& p_transform);
	BBox operator* (nm::float4x4 const& p_transform);
	bool operator==(const BBox& other);
	
	void Merge(const BBox&);
	void Merge(const BSphere&);
	void Reset(Type p_type = Null, Origin p_origin = Auto, nm::float3 p_min = nm::float3(), nm::float3 p_max = nm::float3());

	float GetDepth() const;		// along Z
	float GetHeight() const;	// along Y
	float GetWidth() const;		// along X

	bool isVisiable(BBox);
	bool isVisiable(BSphere);

	nm::Transform GetUnitBBoxTransform() const;

private:
	std::vector<nm::float3> CalculateCorners(nm::float3 p_min, nm::float3 p_max);
	void CalculateMinMaxfromCorners(std::vector<nm::float3> p_corners, nm::float3& p_min, nm::float3& p_max, nm::Transform p_transform);
	nm::float4x4 CalculateScale(nm::float3 p_min, nm::float3 p_max);
	nm::float4x4 CalculateTranslate(nm::float3 p_min, nm::float3 p_max);
};

struct BSphere : BVolume
{
	nm::float3					bsCenter;
	float						bsRadius;

	BSphere();
	~BSphere() {};
	
	void Pure() override {};

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
	nm::Transform				transform;
	VertexList					vertexList;
	std::vector<uint32_t>		indicesList;
	std::vector<SubMesh>		submeshes;
	std::vector<BBox>			submeshesBbox;
	BBox						bbox;

	MeshRaw(): 
		  vertexList(VertexList(Vertex::AttributeFlag::position)) 
		, transform(nm::Transform())
	{};
};
typedef std::vector<MeshRaw> MeshRawList;

struct SceneRaw
{
	std::vector<ImageRaw>		textureList;
	MeshRawList					meshList;
	std::vector<Material>		materialsList;
	uint32_t					materialOffset;
	uint32_t					textureOffset;
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
	VertexList vertices;
	std::vector<int> indices;
	std::vector<int> lineIndices;

	RawSphere() :vertices(VertexList(Vertex::AttributeFlag::position)) {}
};
void GenerateSphere(int p_stackCount, int p_sectorCount, RawSphere& p_sphere, float p_radius = 1.0f);

bool LoadDDS(const char* p_path, ImageRaw& p_data);
bool LoadRawImage(const char* p_path, ImageRaw& p_data);
void FreeRawImage(ImageRaw& p_data);

bool LoadGltf(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData);
bool LoadObj(const char* p_path, SceneRaw& p_objScene, const ObjLoadData& p_loadData);

bool WriteToDisk(const std::filesystem::path& pPath, size_t pDataSize, char* pData);
