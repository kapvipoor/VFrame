#pragma once

#include "VulkanRHI.h"
#include "SceneGraph.h"
#include "AssetLoader.h"
#include "Camera.h"
#include "Light.h"

#include "external/NiceMath.h"

#include <list>

class CCircularList
{
public:
	CCircularList(size_t p_size)
		: m_size(p_size)
		, m_list(m_size, 0.0f)
		, m_average(0.0f)
	{}

	~CCircularList()
	{
	}

	void Add(float p_val)
	{
		m_list.push_back(p_val);
		if (m_list.size() > m_size)
		{
			m_list.pop_front();
		}
	}

	void Data(float* p_data) 
	{ 
		m_average = 0.0f;
		uint32_t i = 0;
		for (auto& it : m_list)
		{
			p_data[i] = it * 1000.0f;	// (1.0f / it);// *1000.0f;
			m_average += p_data[i];
			i++;
		}
		m_average /= m_list.size();
	}
	
	size_t Size() { return m_size; }
	float Average() { return m_average; }

private:
	size_t m_size;
	std::list<float> m_list;
	float m_average;
};

enum SamplerId
{
	  s_Linear						= 0
	, s_Nearest
	, s_max
};

enum BindingSet
{
	  bs_Primary					= 0
	, bs_Scene						= 1	, bs_UI						= 1 ,	bs_DebugDisplay = 1
};

enum BindingDest
{	  // Set 0 - Primary				  // Set 1 - Scene				  // Set 1 - UI		      // Set 1 - DebugDraw
	  bd_Gloabl_Uniform				= 0	, bd_Scene_MeshInfo_Uniform	= 0	, bd_UI_TexArray	= 0 , bd_Debug_Transforms_Uniform = 0
	, bd_Linear_Sampler				= 1	, bd_Env_Specular			= 1	, bd_UI_max
	, bd_Nearest_Sampler			= 2 , bd_Env_Diffuse			= 2
	, bd_ObjPicker_Storage			= 3	, bd_Brdf_Lut				= 3
	, bd_SSAOKernel_Storage			= 4 , bd_Material_Storage		= 4
	, bd_PrimaryRead_TexArray		= 5 , bd_Scene_Lights			= 5
	, bd_RTs_StorageImages			= 6 , bd_Scene_TLAS				= 6
	, bd_RTs_SampledImages			= 7 , bd_SceneRead_TexArray		= 7					
	, bd_Primary_max				= 8 , bd_Scene_max				= 8			
};

struct LoadedUpdateData
{
	uint32_t							swapchainIndex;
	float								timeElapsed;
	nm::float2							screenRes;
	nm::float2							curMousePos;
	bool								isLeftMouseDown;
	bool								isRightMouseDown;
	VkCommandPool						commandPool;
	nm::float4x4						camProjection;
	nm::float4x4						camView;
	CCamera::UpdateData					cameraData;

};

class CDescriptor
{
public:
	struct Descriptor
	{
		CVulkanRHI::DescDataList		descDataList;
		VkDescriptorSetLayout			descLayout;
		VkDescriptorSet					descSet;
	};

	// by default the number of descriptor sets created is 1
	// but there can be more than 1 descDataList and descriptor set pairs
	// provided they are based on the same descriptor set layout and 
	// created from the same Descriptor pool
	CDescriptor(CVulkanRHI::DescriptorBindFlags isBindless = CVulkanRHI::DescriptorBindFlag::Traditonal, uint32_t p_descSetCount = 1);
	~CDescriptor() {};

	void BindlessWrite(uint32_t p_swId, uint32_t p_index, const VkDescriptorImageInfo* p_imageInfo, uint32_t p_count = 1, uint32_t p_arrayDestIndex = 0);
	void BindlessWrite(uint32_t p_swId, uint32_t p_index, const VkDescriptorBufferInfo* p_bufferInfo, uint32_t p_count = 1);
	void BindlessWrite(uint32_t p_swId, uint32_t p_index, const VkAccelerationStructureKHR* p_accStructure, uint32_t p_count = 1);
	void BindlessUpdate(CVulkanRHI* p_rhi, uint32_t p_swId);

	const VkDescriptorSet* GetDescriptorSet(uint32_t p_id = 0) const{ return &m_descList[p_id].descSet; }
	const VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t p_id = 0) const	{ return m_descList[p_id].descLayout; }
protected:
	std::vector<Descriptor>				m_descList;
	VkDescriptorPool					m_descPool;

	bool CreateDescriptors(CVulkanRHI* p_rhi, uint32_t p_id = 0, std::string p_debugName = "");
	void DestroyDescriptors(CVulkanRHI* p_rhi);
	void AddDescriptor(CVulkanRHI::DescriptorData p_dData, uint32_t p_id = 0);

	CVulkanRHI::DescriptorBindFlags m_bindFlags;
};

class CBuffers
{
public:
	// Use this when creating buffers with assorted usage and memory properties
	CBuffers(int p_maxSize = 0);

	// Use this when creating buffers that all have the usage and memory properties
	CBuffers(VkBufferUsageFlags, VkMemoryPropertyFlags, int p_maxSize = 0);

	~CBuffers() {};

	virtual void DestroyBuffers(CVulkanRHI*);

	// Use this to create buffers with assorted usage and memory properties
	bool CreateBuffer(CVulkanRHI*, VkBufferUsageFlags, VkMemoryPropertyFlags, size_t, std::string p_debugName, int32_t p_id = -1);
	
	// Use this to create buffers on Host (Eg: Staging or Imgui buffers)
	bool CreateBuffer(CVulkanRHI*, void* p_data, size_t p_size, std::string p_debugName, int32_t p_id = -1);

	// Use this to create empty buffers on Host or Device
	bool CreateBuffer(CVulkanRHI*, size_t p_size, std::string p_debugName, int32_t p_id = -1);

	// Use this to create buffers on Device (Eg: All GPU resources like Vertex, Index, etc)
	bool CreateBuffer(CVulkanRHI*, CVulkanRHI::Buffer& stg, void* p_data, size_t p_size, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugName, int32_t p_id = -1);

	void DestroyBuffer(CVulkanRHI*, uint32_t p_idx);

	//void AddUsageFlags(VkBufferUsageFlags);
	//void AddMemPropFlags(VkMemoryPropertyFlags);

	VkBufferUsageFlags GetUsage() { return m_usageFlags; }
	VkMemoryPropertyFlags GetMemoryProperties() { return m_memPropFlags; }

	const CVulkanRHI::Buffer& GetBuffer(uint32_t p_id = 0);
	const CVulkanRHI::Buffer GetBuffer(uint32_t p_id = 0) const;

protected:
	CVulkanRHI::BufferList			m_buffers;
	VkBufferUsageFlags				m_usageFlags;
	VkMemoryPropertyFlags			m_memPropFlags;
};

class CTextures
{
public:
	// reserving a default size of 1 if the texture list is expected to group at runtime
	CTextures(int p_maxSize = 0);
	~CTextures() {};

	bool CreateRenderTarget(CVulkanRHI* p_rhi, uint32_t p_id, VkFormat p_format,uint32_t p_width, uint32_t p_height, uint32_t p_mipLevel, VkImageLayout p_layout, std::string p_debugName, VkImageUsageFlags p_usage);
	bool CreateTexture(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& stg, const ImageRaw*, VkFormat p_format, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugName, int p_id = -1);
	bool CreateCubemap(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, ImageRaw&, const CVulkanRHI::SamplerList& p_samplers, CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugName, int p_id = -1);

	// Pushes a specific texture index into list to repeat the usage of the texture
	// Note: this does not reload the texture, but simply makes the texture handle available 
	// at this index that is referenced by the mesh
	void PushBackPreLoadedTexture(uint32_t p_texIndex);

	void DestroyTextures(CVulkanRHI* p_rhi);

	void IssueLayoutBarrier(CVulkanRHI* p_rhi, CVulkanRHI::ImageLayout p_imageLayout, const CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id, int p_mipLevel = -1);
	void IssueMemoryBarrier(CVulkanRHI* p_rhi, VkAccessFlags p_srcAcc, VkAccessFlags p_dstAcc, VkPipelineStageFlags p_scrStg, VkPipelineStageFlags p_destStg, const CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id);

	const CVulkanRHI::Image& GetTexture(uint32_t p_id) { return m_textures[p_id]; }
	const CVulkanRHI::Image GetTexture(uint32_t p_id) const { return m_textures[p_id]; }
	const CVulkanRHI::ImageList& GetTextures() { return m_textures; };

protected:
	CVulkanRHI::ImageList			m_textures;
};

class CFixedBuffers : public CBuffers, public CUIParticipant
{
public:
	enum FixedBufferId
	{
		  fb_PrimaryUniform_0		= 0
		, fb_PrimaryUniform_1
		, fb_ObjectPickerRead
		, fb_ObjectPickerWrite
		, fb_DebugUniform_0
		, fb_DebugUniform_1
		, fb_max
	};

	struct PrimaryUniformData
	{
		int							pingPongIndex;
		nm::float3					cameraLookFrom;
		nm::float4x4				cameraViewProj;
		nm::float4x4				cameraJitteredViewProj;
		nm::float4x4				cameraInvViewProj;
		nm::float4x4				cameraPreViewProj;
		nm::float4x4				cameraProj;
		nm::float4x4				cameraView;
		nm::float4x4				cameraInvView;
		nm::float4x4				cameraInvProj;
		nm::float4x4				skyboxModelView;
		nm::float2					mousePos;
		nm::float2					ssaoNoiseScale;
		float						ssaoKernelSize;
		float						ssaoRadius;
		uint32_t					enable_Shadow_RT_PCF;
		float						shadowTemporalAccumWeight;
		uint32_t					frameCount;
		int							enableIBL;
		float						pbrAmbientFactor;
		int							enableSSAO;
		float						biasSSAO;
		float						ssrEnable;
		float						ssrMaxDistance;
		float						ssrResolution;
		float						ssrThickness;
		float						ssrSteps;
		float						taaResolveWeight;
		float						taaUseMotionVectors;
		float						taaFlickerCorectionMode;
		float						taaReprojectionFilter;
		float						toneMappingExposure;
		float						toneMappingSelection;
	};

	CFixedBuffers();
	~CFixedBuffers();

	bool Create(CVulkanRHI*);
	virtual void Destroy(CVulkanRHI*);

	PrimaryUniformData* GetPrimaryUnifromData() { return &m_primaryUniformData; }

	bool Update(CVulkanRHI*, uint32_t p_scId);

	virtual void Show(CVulkanRHI* p_rhi) override;

private:
	PrimaryUniformData				m_primaryUniformData;
};

struct FixedUpdateData
{
	int	swapchainIndex;
};

class CRenderTargets : public CTextures, public CUIParticipant
{
public:
	enum RenderTargetId
	{
		  rt_PingPong_Depth_0		= 0		// This frame's depth target for History/Current use
		, rt_PingPong_Depth_1		= 1		// This frame's depth target for History/Current use
		, rt_Position				= 2		// Position in view space for Current use
		, rt_PingPong_Normal_0		= 3		// Normal in view space for History/Current use
		, rt_PingPong_Normal_1		= 4		// Normal in view space for History/Current use
		, rt_Albedo					= 5		// Albedo
		, rt_SSAO_Blur				= 6		// SSAO and Blur 
		, rt_DirectionalShadowDepth	= 7		// Directional shadow depth
		, rt_PrimaryColor			= 8		// This frame's color target, copies to swap chain by eof
		, rt_RoughMetal				= 9		// Roughness and Metal
		, rt_Motion					= 10	// Motion vectors
		, rt_SSReflection			= 11	// Screen Space Reflection (RGB)
		, rt_SSRBlur				= 12	// Blurred Screen Space Reflection (for rough surfaces)
		, rt_History_PrimaryColor	= 13	// Previous frame's color target (done post tone mapping)
		, rt_RTShadowTemporalAcc	= 14	// Temporal Accumulated ray traced shadows (4 count - 1 directional + 3 point)
		, rt_RTShadowDenoise		= 15	// Denoised ray traces shadows (4 count - 1 directional + 3 point)
		, rt_max
	};

	CRenderTargets();
	~CRenderTargets();

	bool Create(CVulkanRHI*);
	void Destroy(CVulkanRHI*);

	void Show(CVulkanRHI* p_rhi) override;

	// temporary hack - to create the primary descriptor set, the render targets
	// need to be in a specific layout as some of them are required in compute
	// shaders as shader resources. Once the primary descriptors are created all
	// the layout of the render targets are reset to default IE:
	// LAYOUT_UNDEFINED
	void SetLayout(RenderTargetId, VkImageLayout);

	std::string GetRenderTargetIDinString(RenderTargetId);

private:

	std::vector<uint32_t> m_rtID;
};

class CRenderable
{
public:
	struct InData
	{
		void* vertexBufferData;
		size_t vertexBufferSize;

		void* indexBufferData;
		size_t indexBufferSize;
	};

	// Buffer count defaults to 0 so the size of
	// the buffer list can grow dynamically
	CRenderable(VkMemoryPropertyFlags p_memPropFlags, uint32_t p_BufferCount);
	CRenderable(VkBufferUsageFlags p_usageFlags, VkMemoryPropertyFlags p_memPropFlags, uint32_t p_BufferCount);
	~CRenderable() {};

	// This is a clear operation performed between frames
	// For now called for UI only
	void Clear(CVulkanRHI* p_rhi, uint32_t p_idx);

	// This is a clean up operation performed during shut down
	virtual void DestroyRenderable(CVulkanRHI*);

	virtual bool CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stg, const MeshRaw* p_meshRaw,
		CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugStr, int32_t index = -1);

	bool CreateVertexIndexBuffer(CVulkanRHI* p_rhi, const InData& p_inData, std::string p_debugStr, int32_t index = -1);

	const CVulkanRHI::Buffer GetVertexBuffer(uint32_t p_idx = 0) const;
	const CVulkanRHI::Buffer GetIndexBuffer(uint32_t p_idx = 0) const;

	uint32_t GetInstanceCount() { return m_instanceCount; }
	uint32_t GetPrimitiveCount() { return m_primitiveCount; }
	uint32_t GetVertexCount() { return m_vertexCount; }
	size_t GetVertexStrideInBytes() { return m_vertexStrideInBytes; }

protected:
	CBuffers						m_vertexBuffers;
	CBuffers						m_indexBuffers;

	uint32_t						m_instanceCount;

	uint32_t						m_primitiveCount;
	uint32_t						m_vertexCount;
	size_t							m_vertexStrideInBytes;
};

class CRayTracingRenderable : public CRenderable
{
public:
	CRayTracingRenderable(VkAccelerationStructureInstanceKHR* p_accStructInstance);
	~CRayTracingRenderable() {};

	virtual void DestroyRenderable(CVulkanRHI*) override;

	bool CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stg, const MeshRaw* p_meshRaw,
		CVulkanRHI::CommandBuffer& p_cmdBfr, std::string p_debugStr, int32_t index = -1);

	bool CreateBuildBLAS(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&, std::string p_debugStr);

	VkAccelerationStructureKHR GetBLAS() { return m_BLAS; }
	const CVulkanRHI::Buffer GetBLASBuffer() { return m_blasBuffer.GetBuffer(0); }

protected:
	CBuffers							m_blasBuffer;
	VkAccelerationStructureKHR			m_BLAS;
	VkAccelerationStructureInstanceKHR* m_accStructInstance;

	void UpdateBLASInstance(CVulkanRHI* p_rhi, const nm::Transform& p_transform);
};

class CRenderableUI : public CRenderable, public CTextures, public CDescriptor, public CUIParticipant, public CSelectionListener
{
public:
	static uint32_t fontID;

	struct UIPushConstant
	{
		float						offset[2];
		float						scale[2];
		uint32_t					binding_textureId;
	};

	struct Guizmo
	{
		enum Type
		{
				Translation			= 0
			,	Rotation			= 1
			,	Scale				= 2
		};
		Type type;
		Guizmo():type(Translation) {}
	};

	CRenderableUI();
	~CRenderableUI();

	void virtual Show(CVulkanRHI* p_rhi) override;

	bool Create(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool);
	virtual void DestroyRenderable(CVulkanRHI* p_rhi) override;

	bool Update(CVulkanRHI* p_rhi, const LoadedUpdateData&);
	bool PreDraw(CVulkanRHI* p_rhi, uint32_t p_scIdx);

private:
	Guizmo								m_guizmo;
	bool								m_showImguiDemo;
	CCircularList						m_latestFPS;
	CUIParticipantManager*				m_participantManager;
		
	bool LoadFonts(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&);
	bool CreateUIDescriptors(CVulkanRHI* p_rhi);
	bool ShowUI(CVulkanRHI* p_rhi);
	bool ShowGuizmo(CVulkanRHI* p_rhi, nm::float4x4 p_camView, nm::float4x4 p_camProjection);
};

class CRenderableMesh : public CRayTracingRenderable, public CEntity
{
	friend class CScene;
public:
	CRenderableMesh(std::string p_name, uint32_t p_meshId, nm::Transform p_modelMat, VkAccelerationStructureInstanceKHR* p_accStructInstance);
	~CRenderableMesh();
	
	virtual void Show(CVulkanRHI* p_rhi) override; //CUIParticipant virtual override

	virtual void SetTransform(CVulkanRHI* p_rhi, nm::Transform p_transform, bool p_bRecomputeSceneBBox) override;

	uint32_t GetMeshId() const { return m_mesh_id; }
	uint32_t GetSubmeshCount() const { return (uint32_t)m_submeshes.size(); }
	const SubMesh* GetSubmesh(uint32_t p_idx) const { return &m_submeshes[p_idx]; }

	void SetSubBoundingBox(BBox p_bbox) { m_subBoundingBoxes.push_back(p_bbox); }
	BBox* GetSubBoundingBox(uint32_t p_id) { return &(m_subBoundingBoxes[p_id]); }
	uint32_t GetSubBoundingBoxCount() { return (uint32_t)m_subBoundingBoxes.size(); }

	int GetSelectedSubMeshId() { return m_selectedSubMeshId; }

private:
	std::vector<SubMesh>			m_submeshes;
	std::vector<BBox>				m_subBoundingBoxes;
	uint32_t						m_mesh_id;
	nm::float4x4					m_viewNormalTransform;
	
	// few members needed for ui
	int								m_selectedSubMeshId;
};

class CRenderableDebug : public CRenderable, public CDescriptor
{
public:
	struct DebugVertex
	{
		nm::float3	pos;
		int			id;
	};

	struct DebugDrawDetails
	{
		uint32_t indexCount;
		uint32_t indexOffset;
		uint32_t vertexOffset;
		uint32_t instanceOffset;
		uint32_t instanceCount;
	};
	typedef std::vector<DebugDrawDetails> DebugDrawDetailsList;

	CRenderableDebug();
	~CRenderableDebug() {};

	bool Create(CVulkanRHI* p_rhi, const CFixedBuffers*, const CVulkanRHI::CommandPool&);
	bool Update();
	void Destroy(CVulkanRHI* p_rhi);
	bool PreDrawInstanced(CVulkanRHI* p_rhi, uint32_t p_scIdx, const CFixedBuffers*, const CSceneGraph*, CVulkanRHI::CommandBuffer&);

	DebugDrawDetails GetBBoxDrawDetails() { return m_bBoxDetails; }
	DebugDrawDetails GetBSphereDrawDetails() { return m_bSphereDetails; }
	DebugDrawDetails GetBFrustumDrawDetails() { return m_bFrustumDetails; }

private: 
	DebugDrawDetails			m_bBoxDetails;
	DebugDrawDetails			m_bSphereDetails;
	DebugDrawDetails			m_bFrustumDetails;
	

	bool CreateDebugDescriptors(CVulkanRHI*, const CFixedBuffers*);
	bool CreateBoxSphereBuffers(CVulkanRHI*, CVulkanRHI::BufferList&, CVulkanRHI::CommandBuffer&);
};

class CScene : public CDescriptor, public CUIParticipant, public CSelectionListener
{
public:
	enum TextureType
	{
		  tt_default				= 0
		, tt_env_specular			= 1
		, tt_env_diffuse			= 2
		, tt_brdfLut				= 3
		, tt_scene					= 4
	};

	struct MeshPushConst
	{
		uint32_t					mesh_id;
		uint32_t					material_id;
	};

	CScene(CSceneGraph*);
	~CScene();

	bool Create(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, const CVulkanRHI::CommandPool& p_cmdPool);
	void Destroy(CVulkanRHI* p_rhi);

	bool UpdateTLAS(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool, uint32_t p_scId);

	virtual void Show(CVulkanRHI* p_rhi) override;

	bool Update(CVulkanRHI* p_rhi, const LoadedUpdateData&);
	void SetSelectedRenderableMesh(int p_id) { m_curSelecteRenderableMesh = p_id; }
	uint32_t GetRenderableMeshCount() const { return (uint32_t)m_meshes.size(); }
	const CRenderableMesh* GetRenderableMesh(uint32_t p_idx) const { return m_meshes[p_idx]; }
	const CRenderable* GetSkyBoxMesh() const { return m_skyBox; }

private:
	enum AssetLoadingState
	{
		als_WaitForRequest = 0
		, als_ReadyToLoad
		, als_Loading
		, als_RequestComplete
	};

	struct AssetLoadingTracker
	{
		AssetLoadingState state;
		float progress;
		std::string log;

		AssetLoadingTracker()
			: state(AssetLoadingState::als_WaitForRequest)
			, progress(0.0f)
			, log("") {}
	};

	VkCommandPool m_cmdPool;

	CSceneGraph* m_sceneGraph;

	// If needed, create another family of meshes that are not identified as 
	// ray tracing resources but will be rendered in the frame.
	CRenderable*							m_skyBox;
	std::vector<CRenderableMesh*>			m_meshes;			// list of all meshes used by the scene
		
	CTextures*                              m_sceneTextures;	// list of all the textures used by the scene
	std::vector<Material>					m_materialsList;	// List of all the materials used by the scene
	
	CLights* m_sceneLights;		// List of all the lights used by the scene

	// Used for Ray Tracing
	std::vector<VkAccelerationStructureInstanceKHR> m_accStructInstances; // List of all BLAS instancing data used for creating TLAS
	CBuffers*								m_tlasBuffers;
	VkAccelerationStructureKHR				m_TLAS;
	CVulkanRHI::Buffer						m_TLASscratchBuffer;
	CVulkanRHI::Buffer						m_instanceBuffer;
	
	// TODO: need to fix the current selected render-able mesh 
	// it is used by object picker pass and is not the best way to do.
	int										m_curSelecteRenderableMesh;
	CVulkanRHI::Buffer						m_meshInfo_uniform[FRAME_BUFFER_COUNT];	// stores all meshes uniform data
	CVulkanRHI::Buffer						m_material_storage;
	CVulkanRHI::Buffer						m_light_storage;						// buffer for holding light count, raw light list data
		
	VkCommandPool							m_assetLoaderCommandPool;				// specially for transfer queues
	AssetLoadingTracker						m_assetLoadingTracker;

	uint32_t m_textureOffset;
	uint32_t m_materialOffset;

	bool LoadDefaultTextures(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&);
	bool LoadDefaultScene(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&, bool p_dumpBinaryToDisk = false);
	bool LoadLights(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&, bool p_dumpBinaryToDisk = false);
	bool LoadTLAS(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&);
	bool UpdateTLAS(CVulkanRHI* p_rhi, CVulkanRHI::CommandBuffer&);

	bool CreateMeshUniformBuffer(CVulkanRHI* p_rhi);
	bool CreateSceneDescriptors(CVulkanRHI* p_rhi);

	bool AddEntity(CVulkanRHI* p_rhi, std::string p_path);
	bool DeleteEntity();

	void DestroyStaging(CVulkanRHI* p_rhi, CVulkanRHI::BufferList&);
};

class CReadOnlyTextures : public CTextures
{
public:
	enum TextureReadOnlyId
	{
		  tr_SSAONoise				= 0
		, tr_max
	};

	CReadOnlyTextures();
	~CReadOnlyTextures() {}

	bool Create(CVulkanRHI*, CFixedBuffers&, CVulkanRHI::CommandPool);

	void Destroy(CVulkanRHI*);

private:
	bool CreateSSAOKernelTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData*, CVulkanRHI::CommandBuffer& p_cmdBfr);
};

class CReadOnlyBuffers : public CBuffers
{
public:
	enum BufferReadOnlyId
	{
		  br_SSAOKernel				= 0
		, br_max
	};

	CReadOnlyBuffers();
	~CReadOnlyBuffers() {}

	bool Create(CVulkanRHI*, CFixedBuffers&, CVulkanRHI::CommandPool);
	void Destroy(CVulkanRHI*);

private:
	bool CreateSSAONoiseBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData*, CVulkanRHI::CommandBuffer& p_cmdBfr);
};

class CLoadableAssets;
class CFixedAssets;
class CPrimaryDescriptors : public CDescriptor
{
public:
	CPrimaryDescriptors();
	~CPrimaryDescriptors();

	bool Create(CVulkanRHI*, CFixedAssets&, const CLoadableAssets&);
	void Destroy(CVulkanRHI*);
private:
	void SetLayoutForDescriptorCreation(CRenderTargets*);
	void UnSetLayoutForDescriptorCreation(CRenderTargets*);
};

class CLoadableAssets
{
public:
	CLoadableAssets(CSceneGraph*);
	~CLoadableAssets();

	bool Create(CVulkanRHI*, const CFixedAssets&, const CVulkanRHI::CommandPool& );
	void Destroy(CVulkanRHI*);

	bool Update(CVulkanRHI*, const LoadedUpdateData&);

	CScene* GetScene() { return &m_scene;}
	const CRenderableUI* GetUI() const { return &m_ui; }
	CReadOnlyTextures* GetReadonlyTextures() { return &m_readOnlyTextures; }
	CReadOnlyBuffers* GetReadonlyBuffers() { return &m_readOnlyBuffers; }
	
	const CReadOnlyTextures* GetReadonlyTextures() const { return &m_readOnlyTextures; }
	const CReadOnlyBuffers* GetReadonlyBuffers() const { return &m_readOnlyBuffers; }

private:
	CScene							m_scene;
	CRenderableUI					m_ui;
	CReadOnlyTextures				m_readOnlyTextures;
	CReadOnlyBuffers				m_readOnlyBuffers;
};

class CFixedAssets : public CUIParticipant
{
public:
	CFixedAssets();
	~CFixedAssets() {};

	bool Create(CVulkanRHI*, const CVulkanRHI::CommandPool&);
	void Destroy(CVulkanRHI*);
	bool Update(CVulkanRHI*, const FixedUpdateData&);

	void Show(CVulkanRHI* p_rhi) override;

	CFixedBuffers* GetFixedBuffers() { return &m_fixedBuffers; }
	const CFixedBuffers* GetFixedBuffers() const { return &m_fixedBuffers; }
	CRenderTargets* GetRenderTargets() { return &m_renderTargets; }
	const CRenderTargets* GetRenderTargets() const { return &m_renderTargets; }
	const CVulkanRHI::SamplerList* GetSamplers() const { return &m_samplers; }
	CRenderableDebug* GetDebugRenderer() { return &m_renderableDebug; }

private:
	CFixedBuffers					m_fixedBuffers;
	CRenderTargets					m_renderTargets;
	CVulkanRHI::SamplerList			m_samplers;
	CRenderableDebug				m_renderableDebug;
	
	bool CreateSamplers(CVulkanRHI*);
};