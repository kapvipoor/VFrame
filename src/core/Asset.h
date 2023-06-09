#pragma once

#include "VulkanRHI.h"
#include "SceneGraph.h"
#include "AssetLoader.h"
#include "external/NiceMath.h"
#include <src/core/Asset.h>
#include <list>

class CCircularList
{
public:
	CCircularList(size_t p_size)
		: m_size(p_size)
		, m_list(m_size, 0.0f)
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
		uint32_t i = 0;
		for (auto& it : m_list)
		{
			p_data[i] = (1.0f / it);// *1000.0f;
			i++;
		}
	}
	size_t Size() { return m_size; }

private:
	size_t m_size;
	std::list<float> m_list;
};

enum SamplerId
{
	  s_Linear						= 0
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
	, bd_Linear_Sampler				= 1	, bd_CubeMap_Texture		= 1	, bd_UI_max
	, bd_ObjPicker_Storage			= 2	, bd_Material_Storage		= 2
	, bd_Depth_Image				= 3	, bd_SceneRead_TexArray		= 3
	, bd_PosGBuf_Image				= 4	, bd_Scene_max				= 4
	, bd_NormGBuf_Image				= 5
	, bd_AlbedoGBuf_Image			= 6
	, bd_SSAO_Image					= 7
	, bd_SSAOBlur_Image				= 8
	, bd_SSAOKernel_Storage			= 9
	, bd_LightDepth_Image			= 10
	, bd_PrimaryColor_Image			= 11
	, bd_PrimaryColor_Texture		= 12
	, bd_DeferredRoughMetal_Image	= 13
	, bd_PrimaryRead_TexArray		= 14
	, bd_Primary_max				= 15
};

struct LoadedUpdateData
{
	uint32_t							swapchainIndex;
	float								timeElapsed;
	nm::float4x4						viewMatrix;
	nm::float2							screenRes;
	nm::float2							curMousePos;
	bool								isLeftMouseDown;
	bool								isRightMouseDown;
};

class CUIParticipant
{
public:
	CUIParticipant();
	~CUIParticipant();

	virtual void Show() = 0;
protected:

	bool m_updated;

	bool Header(const char* caption);
	bool CheckBox(const char* caption, bool* value);
	bool CheckBox(const char* caption, int32_t* value);
	bool RadioButton(const char* caption, bool value);
	//bool InputFloat(const char* caption, float* value, float step, uint32_t precision);
	bool SliderFloat(const char* caption, float* value, float min, float max);
	bool SliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
	bool ComboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
	bool Button(const char* caption);
	void Text(const char* formatstr, ...);
};

class CUIParticipantManager
{
	friend class CUIParticipant;
public:
	CUIParticipantManager();
	~CUIParticipantManager();

	void Show();

private:
	static std::vector<CUIParticipant*> m_uiParticipants;
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

	// by deault the number of descriptor sets created is 1
	// but there can be more than 1 descDataList and descriptor set pairs
	// provided they are based on the same descriptor set layout and 
	// created from the same Descriptor pool
	CDescriptor(uint32_t p_descSetCount = 1) { m_descList.resize(p_descSetCount); };
	~CDescriptor() {};

	const VkDescriptorSet* GetDescriptorSet(uint32_t p_id = 0) const{ return &m_descList[p_id].descSet; }
	const VkDescriptorSetLayout GetDescriptorSetLayout(uint32_t p_id = 0) const	{ return m_descList[p_id].descLayout; }
protected:
	std::vector<Descriptor>				m_descList;
	VkDescriptorPool					m_descPool;

	bool CreateDescriptors(CVulkanRHI* p_rhi, uint32_t p_varDescCount, uint32_t p_texArrayIndex, uint32_t p_id = 0);
	void DestroyDescriptors(CVulkanRHI* p_rhi);
	void AddDescriptor(CVulkanRHI::DescriptorData p_dData, uint32_t p_id = 0);
};

class CBuffers
{
public:
	// rezerving a default size of 1 if the buffer list is expected to group at runtime
	CBuffers(int p_maxSize = 0);
	~CBuffers() {};

	bool CreateBuffer(CVulkanRHI*, uint32_t p_id, VkBufferUsageFlags p_usage, VkMemoryPropertyFlags p_memProp, size_t);
	bool CreateBuffer(CVulkanRHI*, CVulkanRHI::Buffer& stg, void* p_data, size_t p_size, CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id = -1);
	
	const CVulkanRHI::Buffer& GetBuffer(uint32_t p_id) { return m_buffers[p_id]; }
	const CVulkanRHI::Buffer GetBuffer(uint32_t p_id) const { return m_buffers[p_id]; }

protected:
	CVulkanRHI::BufferList			m_buffers;
	void DestroyBuffers(CVulkanRHI*);
};

class CTextures
{
public:
	// rezerving a default size of 1 if the texture list is expected to group at runtime
	CTextures(int p_maxSize = 0);
	~CTextures() {};

	bool CreateRenderTarget(CVulkanRHI* p_rhi, uint32_t p_id, VkFormat p_format,uint32_t p_width, uint32_t p_height, VkImageLayout p_layout, VkImageUsageFlags p_usage);
	bool CreateTexture(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& stg, const ImageRaw*, VkFormat p_format, CVulkanRHI::CommandBuffer& p_cmdBfr, int p_id = -1);
	bool CreateCubemap(CVulkanRHI* p_rhi, CVulkanRHI::Buffer& p_stg, const std::vector<ImageRaw>&, const CVulkanRHI::SamplerList& p_samplers, CVulkanRHI::CommandBuffer& p_cmdBfr, int p_id = -1);

	void IssueLayoutBarrier(CVulkanRHI* p_rhi, CVulkanRHI::ImageLayout p_imageLayout, CVulkanRHI::CommandBuffer& p_cmdBfr, uint32_t p_id);

	const CVulkanRHI::Image& GetTexture(uint32_t p_id) { return m_textures[p_id]; }
	const CVulkanRHI::Image GetTexture(uint32_t p_id) const { return m_textures[p_id]; }

protected:
	CVulkanRHI::ImageList			m_textures;
	
	void DestroyTextures(CVulkanRHI* p_rhi);
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
		float						elapsedTime;
		nm::float3					cameraLookFrom;
		nm::float4x4				cameraViewProj;
		nm::float4x4				cameraProj;
		nm::float4x4				cameraView;
		nm::float4x4				cameraInvView;
		nm::float4x4				skyboxModelView;
		nm::float2					renderRes;
		nm::float2					mousePos;
		nm::float2					ssaoNoiseScale;
		float						ssaoKernelSize;
		float						ssaoRadius;
		nm::float4x4				sunViewProj;
		nm::float3					sunDirWorldSpace;
		int							enableShadowPCF;
		nm::float3					sunDirViewSpace;
		float						sunIntensity;
		float						pbrAmbientFactor;
		int							enableSSAO;
		float						biasSSAO;
		float						unassigned_1;
	};

	CFixedBuffers();
	~CFixedBuffers();

	bool Create(CVulkanRHI*);
	void Destroy(CVulkanRHI*);

	PrimaryUniformData& GetPrimaryUnifromData() { return m_primaryUniformData; }
	const PrimaryUniformData& GetPrimaryUnifromData() const { return m_primaryUniformData; }
	void SetPrimaryUniformData(const PrimaryUniformData& p_primUniData) { m_primaryUniformData = p_primUniData; }

	bool Update(CVulkanRHI*, uint32_t p_scId);

	virtual void Show() override;

private:
	PrimaryUniformData				m_primaryUniformData;
};

struct FixedUpdateData
{
	int									swapchainIndex;
	CFixedBuffers::PrimaryUniformData*	primaryUniData;
	CSceneGraph*						sceneGraph;
};

class CRenderTargets : public CTextures
{
public:
	enum RenderTargetId
	{
		  rt_PrimaryDepth			= 0
		, rt_Position				= 1
		, rt_Normal					= 2
		, rt_Albedo					= 3
		, rt_SSAO					= 4
		, rt_SSAOBlur				= 5
		, rt_DirectionalShadowDepth	= 6
		, rt_PrimaryColor			= 7
		, rt_DeferredRoughMetal		= 8
		, rt_max
	};

	CRenderTargets();
	~CRenderTargets();

	bool Create(CVulkanRHI*);
	void Destroy(CVulkanRHI*);

	// temporary hack - to create the primary descriptor set, the rendertargets need to be in a specific layout
	// as some of them are reqused in compute shaders as shader resources. Once the primary descriptors are created
	// all the layout of the render targets are reset to default ie: LAYOUT_UNDEFINED
	void SetLayout(RenderTargetId, VkImageLayout);

private:
};

class CRenderable
{
public:
	CRenderable(uint32_t p_BufferCount = 0);
	~CRenderable() {};

	// This is a aclear operation performed between frames
	// For now called for UI only
	void Clear(CVulkanRHI* p_rhi, uint32_t p_idx);

	// This is a clean up operation performed during shut down
	void DestroyRenderable(CVulkanRHI*);

	bool CreateVertexIndexBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stg, const MeshRaw* p_meshRaw, CVulkanRHI::CommandBuffer& p_cmdBfr);

	void SetVertexBuffer(CVulkanRHI::Buffer p_vertBuf, uint32_t p_idx = 0) { m_vertexBuffers[p_idx] = p_vertBuf; }
	void SetIndexBuffer(CVulkanRHI::Buffer p_indxBuf, uint32_t p_idx = 0) { m_indexBuffers[p_idx] = p_indxBuf; }

	const CVulkanRHI::Buffer* GetVertexBuffer(uint32_t p_idx = 0) const { return &m_vertexBuffers[p_idx]; };
	const CVulkanRHI::Buffer* GetIndexBuffer(uint32_t p_idx = 0) const { return &m_indexBuffers[p_idx]; };

protected:
	CVulkanRHI::BufferList			m_vertexBuffers;
	CVulkanRHI::BufferList			m_indexBuffers;
};

class CRenderableUI : public CRenderable, public CTextures, public CDescriptor, public CSelectionListener
{
public:
	struct UIPushConstant
	{
		float						offset[2];
		float						scale[2];
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

	bool Create(CVulkanRHI* p_rhi, const CVulkanRHI::CommandPool& p_cmdPool);
	void Destroy(CVulkanRHI* p_rhi);

	bool Update(CVulkanRHI* p_rhi, const LoadedUpdateData& , CFixedBuffers::PrimaryUniformData& );
	bool PreDraw(CVulkanRHI* p_rhi, uint32_t p_scIdx);

private:
	Guizmo								m_guizmo;
	bool								m_showImguiDemo;
	CCircularList						m_latestFPS;
	CUIParticipantManager*				m_participantManager;
	CFixedBuffers::PrimaryUniformData	m_primUniforms;
	
	bool LoadFonts(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&);
	bool CreateUIDescriptors(CVulkanRHI* p_rhi);
	bool ShowUI(CVulkanRHI* p_rhi);
	bool ShowGuizmo(CVulkanRHI* p_rhi);
};

class CRenderableMesh : public CRenderable, public CEntity
{
	friend class CScene;
public:
	CRenderableMesh(std::string p_name, uint32_t p_meshId, nm::float4x4 p_modelMat);
	~CRenderableMesh() {};
	
	uint32_t GetMeshId() const { return m_mesh_id; }
	uint32_t GetSubmeshCount() const { return (uint32_t)m_submeshes.size(); }
	const SubMesh* GetSubmesh(uint32_t p_idx) const { return &m_submeshes[p_idx]; }

private:
	std::vector<SubMesh>			m_submeshes;
	uint32_t						m_mesh_id;
	nm::float4x4					m_viewNormalTransform;
};

class CRenderableDebug : public CRenderable, public CDescriptor
{
public:
	struct DebugVertex
	{
		nm::float3	pos;
		int			id;
	};

	CRenderableDebug();
	~CRenderableDebug() {};

	bool Create(CVulkanRHI* p_rhi, const CFixedBuffers*);
	bool Update();
	void Destroy(CVulkanRHI* p_rhi);
	bool PreDraw(CVulkanRHI* p_rhi, uint32_t p_scIdx, const CFixedBuffers*, const CSceneGraph*);
	size_t GetIndexBufferCount() { return m_indexCount; }

private: 
	size_t m_indexCount;
	bool CreateDebugDescriptors(CVulkanRHI*, const CFixedBuffers*);
};

class CScene : public CTextures, public CDescriptor, public CUIParticipant, public CSelectionListener
{
public:
	enum MeshType
	{
		  mt_Skybox					= 0
		, mt_Scene
	};

	enum TextureType
	{
		  tt_default				= 0
		, tt_skybox					= 1
		, tt_scene					= 2
	};

	struct MeshPushConst
	{
		uint32_t					mesh_id;
		uint32_t					material_id;
	};

	CScene(CSceneGraph*);
	~CScene() {};

	bool Create(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList, const CVulkanRHI::CommandPool& p_cmdPool);
	void Destroy(CVulkanRHI* p_rhi);

	virtual void Show() override;

	bool Update(CVulkanRHI* p_rhi, const LoadedUpdateData&);
	void SetSelectedRenderableMesh(int p_id) { m_curSelecteRenderableMesh = p_id; }
	uint32_t GetRenderableMeshCount() const { return (uint32_t)m_meshes.size(); }
	const CRenderableMesh* GetRenderableMesh(uint32_t p_idx) const { return m_meshes[p_idx]; }

private:
	CSceneGraph*							m_sceneGraph;
	std::vector<CRenderableMesh*>			m_meshes;						// list of all meshes required by the scene
	std::vector<Material>					m_materialsList;

	// todo: need to fix the current selected renderable mesh 
	// it is used by object picker pass and is not the best way to do.
	int										m_curSelecteRenderableMesh;		

	CVulkanRHI::Buffer						m_meshInfo_uniform;				// stores all meshes uniform data
	CVulkanRHI::Buffer						m_material_storage;
		
	std::vector<std::filesystem::path>		m_scenePaths;					// list of all scene paths

	bool LoadDefaultTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&);
	bool LoadSkybox(CVulkanRHI* p_rhi, const CVulkanRHI::SamplerList* p_samplerList , CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&);
	bool LoadScene(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgbufferList, CVulkanRHI::CommandBuffer&, bool p_dumpBinaryToDisk = false);

	bool CreateMeshUniformBuffer(CVulkanRHI* p_rhi);
	bool CreateSceneDescriptors(CVulkanRHI* p_rhi);
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
	bool CreateSSAOKernelTexture(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData&, CVulkanRHI::CommandBuffer& p_cmdBfr);
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
	bool CreateSSAONoiseBuffer(CVulkanRHI* p_rhi, CVulkanRHI::BufferList& p_stgList, CFixedBuffers::PrimaryUniformData&, CVulkanRHI::CommandBuffer& p_cmdBfr);
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

	bool Update(CVulkanRHI*, const LoadedUpdateData&, CFixedBuffers::PrimaryUniformData&);

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

class CFixedAssets
{
public:
	CFixedAssets() {};
	~CFixedAssets() {};

	bool Create(CVulkanRHI*);
	void Destroy(CVulkanRHI*);
	bool Update(CVulkanRHI*, const FixedUpdateData&);

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
};