#pragma once

#include "core/WinCore.h"
#include "core/VulkanRHI.h"
#include "core/Camera.h"
#include "core/SceneGraph.h"
#include "core/Asset.h"

#include "LightingPass.h"
#include "ScreenSpacePass.h"
#include "UIPass.h"
#include "PostProcessingPasses.h"

#include "core/Global.h"

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
		  cb_Shadows				= 0
		, cb_SSAO					= 1
		, cb_Forward				= 2
		, cb_Deferred_GBuf			= 3
		, cb_Deferred_Lighting		= 4
		, cb_DebugDraw				= 5
		, cb_UI						= 6
		, cb_PickerCopy2CPU			= 7
		, cb_ToneMapping			= 8
		, cb_Skybox					= 9
		, cb_SSR					= 10
		, cb_TAA					= 11
		, cb_max
	};

	enum FragTexType
	{
		  ft_diffuse				= 0
		, ft_normal
		, ft_max
	};

	uint32_t							m_swapchainIndex;
	uint32_t							m_pingPongIndex;
	uint32_t							m_frameCount;

	VkSemaphore							m_activeAcquireSemaphore;
	VkSemaphore							m_vkswapchainAcquireSemaphore[FRAME_BUFFER_COUNT];
	VkSemaphore							m_vksubmitCompleteSemaphore;

	VkFence								m_activeCmdBfrFreeFence;
	VkFence								m_vkFenceCmdBfrFree[FRAME_BUFFER_COUNT];

	VkCommandPool						m_vkCmdPool;
	std::string							m_cmdBufferNames[FRAME_BUFFER_COUNT][CommandBufferId::cb_max];
	CVulkanRHI::CommandBuffer			m_vkCmdBfr[FRAME_BUFFER_COUNT][CommandBufferId::cb_max];
	CVulkanRHI::CommandBufferList		m_cmdBfrsInUse;

	CPerspectiveCamera*					m_primaryCamera;
		
	bool								m_pickObject;

	CVulkanRHI*							m_rhi;
	CFixedAssets*						m_fixedAssets;
	CLoadableAssets*					m_loadableAssets;
	CPrimaryDescriptors*				m_primaryDescriptors;
	CSceneGraph*						m_sceneGraph;

	CShadowPass*						m_shadowPass;
	CSkyboxPass*						m_skyboxForwardPass;
	CSkyboxDeferredPass*				m_skyboxDeferredPass;
	CForwardPass*						m_forwardPass;
	CDeferredPass*						m_deferredPass;
	CSSAOComputePass*					m_ssaoComputePass;
	CSSAOBlurPass*						m_ssaoBlurPass;
	CSSRComputePass*					m_ssrComputePass;
	CSSRBlurPass*						m_ssrBlurPass;
	CTAAComputePass*					m_taaComputePass;
	CDeferredLightingPass*				m_deferredLightPass;
	CDebugDrawPass*						m_debugDrawPass;
	CToneMapPass*						m_toneMapPass;
	CCopyComputePass*					m_copyComputePass;
	CUIPass*							m_uiPass;
	
	VkSemaphore GetAvailableAcquireSemaphore(VkSemaphore p_in);
	VkFence WaitForFinishIfNecessary(VkFence p_in);
	void UpdatePingPongIndex();

	bool InitCamera();
	bool CreateSyncPremitives();																															
	void DestroySyncPremitives();
	bool CreatePasses();

	void UpdateCamera(CCamera::UpdateData&);
	void UpdateSceneGraphDependencies(float p_delta);

	bool DoReadBackObjPickerBuffer(uint32_t p_swapchainIndex, CVulkanRHI::CommandBuffer& p_cmdBfr);

	bool BeginAllCommandBuffers(uint32_t p_swapchainIndex);
	bool EndAllCommandBuffers(uint32_t p_swapchainIndex);

	bool RenderFrame(CVulkanRHI::RendererType p_renderType);
};