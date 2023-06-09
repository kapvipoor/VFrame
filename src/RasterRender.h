#pragma once

#include <unordered_map>
#include "core/WinCore.h"
#include "core/VulkanRHI.h"
#include "core/Camera.h"
#include "core/SceneGraph.h"
#include "core/Asset.h"
#include "core/Light.h"

#include "ShadowPass.h"
#include "LightingPass.h"
#include "SSAOPass.h"
#include "UIPass.h"

class CRasterRender : public CWinCore
{
public:
	CRasterRender(const char* name, int screen_width_, int screen_height_, int window_scale);
	~CRasterRender();

	void on_destroy() override;
	bool on_create(HINSTANCE pInstance) override;
	bool on_update(float delta) override;
	void on_present() override;	

private:
	enum CommandBufferId
	{
		  cb_ShadowMap				= 0
		, cb_SSAO					= 1
		, cb_SSAO_Blur				= 2
		, cb_Forward				= 3
		, cb_Deferred_GBuf			= 4
		, cb_Deferred_Lighting		= 5
		, cb_DebugDraw				= 6
		, cb_UI						= 7
		, cb_PickerCopy2CPU			= 8
		, cb_ToneMapping			= 9
		, cb_Skybox					= 10
		, cb_max
	};

	enum FragTexType
	{
		  ft_diffuse				= 0
		, ft_normal
		, ft_max
	};

	uint32_t							m_swapchainIndex;

	VkSemaphore							m_vkswapchainAcquireSemaphore;
	VkSemaphore							m_vksubmitCompleteSemaphore;
	VkFence								m_vkFenceCmdBfrFree[FRAME_BUFFER_COUNT];

	VkCommandPool						m_vkCmdPool;
	CVulkanRHI::CommandBuffer			m_vkCmdBfr[FRAME_BUFFER_COUNT][CommandBufferId::cb_max];
	CVulkanRHI::CommandBufferList		m_cmdBfrsInUse;

	CPerspectiveCamera*					m_primaryCamera;
	//COrthoCamera*						m_sunLightCamera;
	CDirectionaLight*					m_sunLight;
	//CPerspectiveCamera m_sunLightCamera;
		
	bool								m_pickObject;

	CVulkanRHI*							m_rhi;
	CFixedAssets*						m_fixedAssets;
	CLoadableAssets*					m_loadableAssets;
	CPrimaryDescriptors*				m_primaryDescriptors;
	CSceneGraph*						m_sceneGraph;

	CStaticShadowPrepass*				m_staticShadowPass;
	CSkyboxPass*						m_skyboxForwardPass;
	CSkyboxDeferredPass*				m_skyboxDeferredPass;
	CForwardPass*						m_forwardPass;
	CDeferredPass*						m_deferredPass;
	CSSAOComputePass*					m_ssaoComputePass;
	CSSAOBlurPass*						m_ssaoBlurPass;
	CDeferredLightingPass*				m_deferredLightPass;
	CDebugDrawPass*						m_debugDrawPass;
	CToneMapPass*						m_toneMapPass;
	CUIPass*							m_uiPass;
	
	bool InitCamera();
	bool CreateSyncPremitives();																															
	void DestroySyncPremitives();
	bool CreatePasses();

	void UpdateCamera(float p_delta);	
	void UpdateSceneGraphDependencies(float p_delta);

	bool DoReadBackObjPickerBuffer(uint32_t p_swapchainIndex, CVulkanRHI::CommandBuffer& p_cmdBfr);
	bool RenderForward();
	bool RenderDeferred();
};