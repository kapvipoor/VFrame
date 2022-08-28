#pragma once

#include <unordered_map>
#include "core/WinCore.h"
#include "core/VulkanRHI.h"
#include "core/Camera.h"
#include "core/SceneGraph.h"
#include "core/Asset.h"

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
		, cb_Forward				= 1
		, cb_Deferred_GBuf			= 2
		, cb_SSAO					= 3
		, cb_Deferred_Lighting		= 4
		, cb_DebugDraw				= 5
		, cb_UI						= 6
		, cb_PickerCopy2CPU			= 7
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
	VkCommandBuffer						m_vkCmdBfr[FRAME_BUFFER_COUNT][CommandBufferId::cb_max];
	std::vector<VkCommandBuffer>		m_cmdBfrsInUse;

	CPerspectiveCamera*					m_primaryCamera;
	COrthoCamera*						m_sunLightCamera;
	//CPerspectiveCamera m_sunLightCamera;
		
	bool								m_pickObject;

	CVulkanRHI*							m_rhi;
	CFixedAssets*						m_fixedAssets;
	CLoadableAssets*					m_loadableAssets;
	CPrimaryDescriptors*				m_primaryDescriptors;
	CSceneGraph*						m_sceneGraph;

	CStaticShadowPrepass*				m_staticShadowPass;
	CSkyboxPass*						m_skyboxPass;
	CForwardPass*						m_forwardPass;
	CDeferredPass*						m_deferredPass;
	CSSAOComputePass*					m_ssaoComputePass;
	CSSAOBlurPass*						m_ssaoBlurPass;
	CDeferredLightingPass*				m_deferredLightPass;
	CDebugDrawPass*						m_debugDrawPass;
	CUIPass*							m_uiPass;

	bool InitCamera();
	bool CreateSyncPremitives();																															
	void DestroySyncPremitives();
	bool CreatePasses();

	void UpdateCamera(float p_delta);																																
	bool DoReadBackObjPickerBuffer(uint32_t p_swapchainIndex, CVulkanRHI::CommandBuffer& p_cmdBfr);
};