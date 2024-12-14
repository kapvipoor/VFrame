#pragma once

#include "core/RandGen.h"
#include "RasterRender.h"
#include <assert.h>

//float g_sunDistanceFromOrigin = 50.0f;
//nm::float4 g_sunDirection = nm::float4(0.0f, 1.0f, 0.0f, 0.0f);// -93.83f);

CRasterRender::CRasterRender(const char* name, int screen_width_, int screen_height_, int window_scale)
	: CWinCore(name, screen_width_, screen_height_, window_scale)
	, m_pickObject(false)
	, m_activeAcquireSemaphore(VK_NULL_HANDLE)
	, m_activeCmdBfrFreeFence(VK_NULL_HANDLE)
	, m_frameCount(0)

{			
	m_rhi					= new CVulkanRHI(name, RENDER_RESOLUTION_X, RENDER_RESOLUTION_Y);

	m_primaryCamera			= new CPerspectiveCamera();
	//m_sunLight				= new CDirectionaLight("Sunlight", true, nm::float3(0.0f, 1.0f, 0.0f));

	m_sceneGraph			= new CSceneGraph(m_primaryCamera);

	m_fixedAssets			= new CFixedAssets();
	m_loadableAssets		= new CLoadableAssets(m_sceneGraph);
	m_primaryDescriptors	= new CPrimaryDescriptors();

	m_staticShadowPass		= new CStaticShadowPrepass(m_rhi);
	m_skyboxForwardPass		= new CSkyboxPass(m_rhi);
	m_skyboxDeferredPass	= new CSkyboxDeferredPass(m_rhi);
	m_forwardPass			= new CForwardPass(m_rhi);
	m_deferredPass			= new CDeferredPass(m_rhi);
	m_deferredLightPass		= new CDeferredLightingPass(m_rhi);
	m_ssaoComputePass		= new CSSAOComputePass(m_rhi);
	m_ssaoBlurPass			= new CSSAOBlurPass(m_rhi);
	m_ssrComputePass		= new CSSRComputePass(m_rhi);
	m_debugDrawPass			= new CDebugDrawPass(m_rhi);
	m_toneMapPass			= new CToneMapPass(m_rhi);
	m_uiPass				= new CUIPass(m_rhi);
	m_taaComputePass		= new CTAAComputePass(m_rhi);
	m_copyComputePass		= new CCopyComputePass(m_rhi);

	m_cmdBufferNames[0][cb_CopyCompute]			= "CopyCompute_0";
	m_cmdBufferNames[0][cb_TAA]					= "TAA_0";
	m_cmdBufferNames[0][cb_SSR]					= "SSR_0";
	m_cmdBufferNames[0][cb_ShadowMap]			= "ShadowMap_0";
	m_cmdBufferNames[0][cb_SSAO]				= "SSAO_0";
	m_cmdBufferNames[0][cb_SSAO_Blur]			= "SSAOBlur_0";
	m_cmdBufferNames[0][cb_Forward]				= "Forward_0";
	m_cmdBufferNames[0][cb_Deferred_GBuf]		= "Deferred_GBuf_0";
	m_cmdBufferNames[0][cb_Deferred_Lighting]	= "Deferred_Lighting_0";
	m_cmdBufferNames[0][cb_DebugDraw]			= "DebugDraw_0";
	m_cmdBufferNames[0][cb_UI]					= "UI_0";
	m_cmdBufferNames[0][cb_PickerCopy2CPU]		= "PickerCopy2CPU_0";
	m_cmdBufferNames[0][cb_ToneMapping]			= "ToneMapping_0";
	m_cmdBufferNames[0][cb_Skybox]				= "Skybox_0";

	m_cmdBufferNames[1][cb_CopyCompute]			= "CopyCompute_1";
	m_cmdBufferNames[1][cb_TAA]					= "TAA_1";
	m_cmdBufferNames[1][cb_SSR]					= "SSR_1";
	m_cmdBufferNames[1][cb_ShadowMap]			= "ShadowMap_1";
	m_cmdBufferNames[1][cb_SSAO]				= "SSAO_1";
	m_cmdBufferNames[1][cb_SSAO_Blur]			= "SSAOBlur_1";
	m_cmdBufferNames[1][cb_Forward]				= "Forward_1";
	m_cmdBufferNames[1][cb_Deferred_GBuf]		= "Deferred_GBuf_1";
	m_cmdBufferNames[1][cb_Deferred_Lighting]	= "Deferred_Lighting_1";
	m_cmdBufferNames[1][cb_DebugDraw]			= "DebugDraw_1";
	m_cmdBufferNames[1][cb_UI]					= "UI_1";
	m_cmdBufferNames[1][cb_PickerCopy2CPU]		= "PickerCopy2CPU_1";
	m_cmdBufferNames[1][cb_ToneMapping]			= "ToneMapping_1";
	m_cmdBufferNames[1][cb_Skybox]				= "Skybox_1";
}

CRasterRender::~CRasterRender() 
{
	//delete m_sunLight;
	//delete m_sunLightCamera;
	delete m_primaryCamera;

	delete m_sceneGraph;

	delete m_copyComputePass;
	delete m_taaComputePass;
	delete m_uiPass;
	delete m_toneMapPass;
	delete m_debugDrawPass;
	delete m_ssrComputePass;
	delete m_ssaoBlurPass;
	delete m_ssaoComputePass;
	delete m_deferredLightPass;
	delete m_deferredPass;
	delete m_forwardPass;
	delete m_skyboxDeferredPass;
	delete m_skyboxForwardPass;
	delete m_staticShadowPass;

	delete m_primaryDescriptors;
	delete m_loadableAssets;
	delete m_fixedAssets;

	delete m_rhi;
}

void CRasterRender::on_destroy()
{
	m_uiPass->Destroy();
	m_copyComputePass->Destroy();
	m_taaComputePass->Destroy();
	m_toneMapPass->Destroy();
	m_debugDrawPass->Destroy();
	m_ssrComputePass->Destroy();
	m_ssaoBlurPass->Destroy();
	m_ssaoComputePass->Destroy();
	m_deferredLightPass->Destroy();
	m_deferredPass->Destroy();
	m_forwardPass->Destroy();
	m_skyboxDeferredPass->Destroy();
	m_skyboxForwardPass->Destroy();
	m_staticShadowPass->Destroy();

	m_primaryDescriptors->Destroy(m_rhi);
	m_loadableAssets->Destroy(m_rhi);
	m_fixedAssets->Destroy(m_rhi);

	DestroySyncPremitives();

	m_rhi->cleanUp();
}

bool CRasterRender::on_create(HINSTANCE pInstance)
{
	CVulkanRHI::InitData initData{};
	initData.winInstance							= pInstance;
	initData.winHandle								= CWinCore::s_Window.handle;
	initData.queueType								= VK_QUEUE_GRAPHICS_BIT;
	initData.swapChaineImageUsage					= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	initData.swapchainImageFormat					= VK_FORMAT_B8G8R8A8_UNORM;

	RETURN_FALSE_IF_FALSE(m_rhi->initialize(initData));

	// Will create command pool for Compute Queue Family only 
	// (might be graphics compatible as well)
	RETURN_FALSE_IF_FALSE(m_rhi->CreateCommandPool(m_rhi->GetQueueFamiliyIndex(), m_vkCmdPool));

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		RETURN_FALSE_IF_FALSE(m_rhi->CreateCommandBuffers(m_vkCmdPool, m_vkCmdBfr[i], CommandBufferId::cb_max, m_cmdBufferNames[i]));
	}

	RETURN_FALSE_IF_FALSE(CreateSyncPremitives());

	RETURN_FALSE_IF_FALSE(m_fixedAssets->Create(m_rhi, m_vkCmdPool));

	RETURN_FALSE_IF_FALSE(m_loadableAssets->Create(m_rhi, *m_fixedAssets, m_vkCmdPool));
	
	RETURN_FALSE_IF_FALSE(m_primaryDescriptors->Create(m_rhi, *m_fixedAssets, *m_loadableAssets));

	RETURN_FALSE_IF_FALSE(CreatePasses());

	{
		RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(m_vkCmdBfr[0][0], "Preparing Render Targets"));

		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_vkCmdBfr[0][0], CRenderTargets::rt_PrimaryDepth);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_Position);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_Normal);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_Albedo);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_SSAO_Blur);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_vkCmdBfr[0][0], CRenderTargets::rt_DirectionalShadowDepth);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_PrimaryColor);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_RoughMetal_Motion);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_SSReflection);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_Prev_PrimaryColor);
		
		RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(m_vkCmdBfr[0][0]));

		CVulkanRHI::CommandBufferList cbrList{ m_vkCmdBfr[0][0] };
		CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
		bool waitForFinish = true;
		RETURN_FALSE_IF_FALSE(m_rhi->SubmitCommandBuffers(&cbrList, &psfList, waitForFinish, &m_vkFenceCmdBfrFree[0]));
		RETURN_FALSE_IF_FALSE(m_rhi->ResetFence(m_vkFenceCmdBfrFree[0]));
	}

	RETURN_FALSE_IF_FALSE(InitCamera());

	//m_rhi->SetRenderType(CVulkanRHI::RendererType::Deferred);
	//m_ssrComputePass->Enable(false);
	m_taaComputePass->Enable(false);

	return true;
}

bool CRasterRender::on_update(float delta)
{
	m_activeAcquireSemaphore = GetAvailableAcquireSemaphore(m_activeAcquireSemaphore);
	RETURN_FALSE_IF_FALSE(m_rhi->AcquireNextSwapChain(m_activeAcquireSemaphore, m_swapchainIndex));

	m_sceneGraph->Update();

	CCamera::UpdateData camUpdateData{};
	camUpdateData.timeDelta = delta;
	UpdateCamera(camUpdateData);

	// if left mouse is clicked; prepare and pick object for this frame
	m_pickObject = false; //m_keys[LEFT_MOUSE_BUTTON].down;

	int mousepos_x = 0, mousepos_y = 0;
	GetCurrentMousePosition(mousepos_x, mousepos_y);
	
	{
		CFixedBuffers::PrimaryUniformData uniformData = m_fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData();
		uniformData.cameraInvView					= nm::inverse(m_primaryCamera->GetView());
		uniformData.cameraLookFrom					= m_primaryCamera->GetLookFrom();
		uniformData.cameraProj						= m_primaryCamera->GetProjection();
		uniformData.cameraView						= m_primaryCamera->GetView();
		uniformData.cameraViewProj					= m_primaryCamera->GetViewProj();
		uniformData.cameraJitteredViewProj			= m_primaryCamera->GetJitteredViewProj();
		uniformData.cameraInvViewProj				= m_primaryCamera->GetInvViewProj();
		uniformData.cameraPreViewProj				= m_primaryCamera->GetPreViewProj();
		uniformData.mousePos						= nm::float2((float)mousepos_x, (float)mousepos_y);
		uniformData.skyboxModelView					= m_primaryCamera->GetView();
		uniformData.ssrEnable						= m_ssrComputePass->IsEnabled();
		uniformData.ssrMaxDistance					= m_ssrComputePass->GetMaxDistance();
		uniformData.ssrResolution					= m_ssrComputePass->GetResolution();
		uniformData.ssrThickness					= m_ssrComputePass->GetThickness();
		uniformData.ssrSteps						= m_ssrComputePass->GetSteps();
		uniformData.taaResolveWeight				= m_taaComputePass->GetResolveWeight();
		uniformData.taaUseMotionVectors				= m_taaComputePass->UseMotionVectors();
		uniformData.taaFlickerCorectionMode			= (float)m_taaComputePass->GetFlickerCorrectionMode();
		uniformData.taaReprojectionFilter			= (float)m_taaComputePass->GetReprojectionFilter();
		uniformData.toneMappingSelection			= (float)m_toneMapPass->GetActiveToneMapper();
		uniformData.toneMappingExposure				= m_toneMapPass->GetExposure();

		FixedUpdateData fixedUpdate{};
		fixedUpdate.primaryUniData					= &uniformData;
		fixedUpdate.swapchainIndex					= m_swapchainIndex;
		fixedUpdate.sceneGraph						= m_sceneGraph;
		RETURN_FALSE_IF_FALSE(m_fixedAssets->Update(m_rhi, fixedUpdate));
	}
	
	{
		LoadedUpdateData loadedUpdate{};
		loadedUpdate.curMousePos					= nm::float2((float)mousepos_x, (float)mousepos_y);
		loadedUpdate.isLeftMouseDown				= m_keys[LEFT_MOUSE_BUTTON].down;
		loadedUpdate.isRightMouseDown				= m_keys[RIGHT_MOUSE_BUTTON].down;
		loadedUpdate.screenRes						= nm::float2((float)s_Window.screenWidth, (float)s_Window.screenHeight);
		loadedUpdate.swapchainIndex					= m_swapchainIndex;
		loadedUpdate.timeElapsed					= delta;
		loadedUpdate.viewMatrix						= m_primaryCamera->GetView();
		loadedUpdate.commandPool					= m_vkCmdPool;
		loadedUpdate.cameraData						= camUpdateData;

		RETURN_FALSE_IF_FALSE(m_loadableAssets->Update(m_rhi, loadedUpdate, m_fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData()));
	}

	{
		CPass::UpdateData updateData{};
		updateData.sceneGraph						= m_sceneGraph;

		m_staticShadowPass->Update(&updateData);
		m_skyboxForwardPass->Update(&updateData);
		m_skyboxDeferredPass->Update(&updateData);
		m_ssaoComputePass->Update(&updateData);
		m_ssaoBlurPass->Update(&updateData);
		m_ssrComputePass->Update(&updateData);
		m_forwardPass->Update(&updateData);
		m_deferredPass->Update(&updateData);
		m_deferredLightPass->Update(&updateData);
		m_debugDrawPass->Update(&updateData);
		m_toneMapPass->Update(&updateData);
		m_taaComputePass->Update(&updateData);
		m_uiPass->Update(&updateData);
	}
	
	RenderFrame(m_rhi->GetRendererType());

	if (m_pickObject)
	{
		m_cmdBfrsInUse.push_back(m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_PickerCopy2CPU]);
		if (!DoReadBackObjPickerBuffer(m_swapchainIndex, m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_PickerCopy2CPU]))
			return false;
	}

	CVulkanRHI::PipelineStageFlagsList psfList{ VkPipelineStageFlags {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} };
	bool waitForFinish = false;
	bool waitForFence = false;
	CVulkanRHI::SemaphoreList signalList{ m_vksubmitCompleteSemaphore };
	CVulkanRHI::SemaphoreList waitList{ m_activeAcquireSemaphore };
	RETURN_FALSE_IF_FALSE(m_rhi->SubmitCommandBuffers(
		&m_cmdBfrsInUse, &psfList, waitForFinish, VK_NULL_HANDLE/*&m_vkFenceCmdBfrFree[m_swapchainIndex]*/, 
		waitForFence, &signalList, &waitList));

	m_cmdBfrsInUse.clear();

	if (m_pickObject)
	{
		int meshID = -1;
		RETURN_FALSE_IF_FALSE(m_rhi->ReadFromBuffer((uint8_t*)&meshID, m_fixedAssets->GetFixedBuffers()->GetBuffer(CFixedBuffers::fb_ObjectPickerRead)));
		m_loadableAssets->GetScene()->SetSelectedRenderableMesh(meshID);
		std::clog << "Picked Mesh: " << (meshID - 1) << std::endl;
	}

	return true;
}

void CRasterRender::on_present()
{
	CVulkanRHI::SwapChain swapchain				= m_rhi->GetSwapChain();
	CVulkanRHI::Queue queue						= m_rhi->GetQueue(0);

	VkPresentInfoKHR presentInfo{};
	VkResult presentResult						= VkResult::VK_RESULT_MAX_ENUM;
	presentInfo.sType							= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount				= 1;
	presentInfo.pWaitSemaphores					= &m_vksubmitCompleteSemaphore;
	presentInfo.swapchainCount					= 1;
	presentInfo.pSwapchains						= &swapchain;
	presentInfo.pImageIndices					= &m_swapchainIndex;
	presentInfo.pResults						= &presentResult;
	VkResult res = vkQueuePresentKHR(queue, &presentInfo);
	if (res != VK_SUCCESS)
	{
		std::cerr << "CRasterRender::on_present Error: vkQueueSubmit failed " << res << std::endl;
	}

	m_rhi->WaitToFinish(queue);
	++m_frameCount;
}

VkSemaphore CRasterRender::GetAvailableAcquireSemaphore(VkSemaphore p_in)
{
	return (p_in == m_vkswapchainAcquireSemaphore[1] || p_in == VK_NULL_HANDLE) ? m_vkswapchainAcquireSemaphore[0] : m_vkswapchainAcquireSemaphore[1];
}

VkFence CRasterRender::WaitForFinishIfNecessary(VkFence p_in)
{
	if (p_in == m_vkFenceCmdBfrFree[0])
	{
		if (m_swapchainIndex == 0) 
		{
			m_rhi->WaitFence(p_in);
			m_rhi->ResetFence(p_in);
			return m_vkFenceCmdBfrFree[1];
		}
		return p_in;
	}
		
	if (p_in == m_vkFenceCmdBfrFree[1])
	{
		if(m_swapchainIndex == 1)
		{
			m_rhi->WaitFence(p_in);
			m_rhi->ResetFence(p_in);
			return m_vkFenceCmdBfrFree[0];
		}
		return p_in;
	}

	if (p_in == VK_NULL_HANDLE)
		return m_vkFenceCmdBfrFree[0];

	return p_in;
}

bool CRasterRender::InitCamera()
{
	CPerspectiveCamera::PerpspectiveInitdData persIntData{};
	persIntData.fov								= 45.0f;
	persIntData.aspect							= (float)CWinCore::s_Window.screenWidth / CWinCore::s_Window.screenHeight;
	persIntData.lookFrom						= nm::float4(0.0f, 0.0f, 4.0f, 1.0f);
	persIntData.lookAt							= nm::float3{ 0.0f, 0.0f, 0.0f };
	persIntData.up								= nm::float3{ 0.0f, 1.0f,  0.0f };
	persIntData.aperture						= 0.1f;							
	persIntData.focusDistance					= 10.0f;							
	persIntData.yaw								= 0.0f;									
	persIntData.pitch							= 0.0f;							
	RETURN_FALSE_IF_FALSE( m_primaryCamera->Init(&persIntData));

	return true;
}

bool CRasterRender::CreateSyncPremitives()
{
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		RETURN_FALSE_IF_FALSE(m_rhi->CreateSemaphor(m_vkswapchainAcquireSemaphore[i], "Acquire Swap Chain Semaphore " + std::to_string(i)));
		RETURN_FALSE_IF_FALSE(m_rhi->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT, m_vkFenceCmdBfrFree[i], "Cmd Bfr Free Fence " + std::to_string(1)));
		m_rhi->ResetFence(m_vkFenceCmdBfrFree[i]);
	}

	RETURN_FALSE_IF_FALSE(m_rhi->CreateSemaphor(m_vksubmitCompleteSemaphore, "Gfx Queue Submit Complete Semaphore"));

	return true;
}

void CRasterRender::DestroySyncPremitives()
{
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		m_rhi->DestroySemaphore(m_vkswapchainAcquireSemaphore[i]);
		m_rhi->DestroyFence(m_vkFenceCmdBfrFree[i]);
	}
	m_rhi->DestroySemaphore(m_vksubmitCompleteSemaphore);
}

bool CRasterRender::CreatePasses()
{
	// Push Constants for G Buffers
	VkPushConstantRange meshPushrange{};
	meshPushrange.offset					= 0;
	meshPushrange.size						= sizeof(CScene::MeshPushConst);	// pushing mesh and sub-mesh ID
	meshPushrange.stageFlags				= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Push constants for UI
	VkPushConstantRange uiPushrange{};
	uiPushrange.offset						= 0;
	uiPushrange.size						= sizeof(CRenderableUI::UIPushConstant);
	uiPushrange.stageFlags					= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Push constants for debug draw
	VkPushConstantRange debugDrawPushrange{};
	debugDrawPushrange.offset				= 0;
	debugDrawPushrange.size					= sizeof(uint32_t);
	debugDrawPushrange.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayout primaryAndSceneLayout;
	std::vector<VkDescriptorSetLayout> descLayouts;
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_loadableAssets->GetScene()->GetDescriptorSetLayout());			// Set 1 - BindingSet::Mesh
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&meshPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), primaryAndSceneLayout, "PrimaryAndScenePipelineLayout"));

	VkPipelineLayout primaryLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&meshPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), primaryLayout, "PrimaryPipelineLayout"));

	VkPipelineLayout uiLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_loadableAssets->GetUI()->GetDescriptorSetLayout());				// Set 1 - BindingSet::UI
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&uiPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), uiLayout, "UIPipelineLayout"));

	VkPipelineLayout debugLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_fixedAssets->GetDebugRenderer()->GetDescriptorSetLayout());		// Set 1 - BindingSet::Debug
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&debugDrawPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), debugLayout, "DebugPipelineLayout"));


	CPass::RenderData renderData{};
	renderData.fixedAssets					= m_fixedAssets;
	renderData.loadedAssets					= m_loadableAssets;
	renderData.rendererType					= m_rhi->GetRendererType();

	CVulkanRHI::Pipeline pipeline;
	pipeline.pipeLayout						= primaryAndSceneLayout;
	pipeline.cullMode						= VK_CULL_MODE_BACK_BIT;		// to fix peter-panning issue
	pipeline.enableDepthTest				= true;
	pipeline.enableDepthWrite				= true;
	RETURN_FALSE_IF_FALSE(m_staticShadowPass->Initalize(&renderData, pipeline));
	CVulkanRHI::VertexBinding vertexBindinginUse;
	m_staticShadowPass->GetVertexBindingInUse(vertexBindinginUse);

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryAndSceneLayout;
	pipeline.vertexInBinding				= vertexBindinginUse.bindingDescription;
	pipeline.vertexAttributeDesc			= vertexBindinginUse.attributeDescription;
	RETURN_FALSE_IF_FALSE(m_forwardPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.renderpassData					= m_forwardPass->GetRenderpass();			// reusing render-pass from forward pass
	pipeline.pipeLayout						= primaryAndSceneLayout;
	pipeline.vertexInBinding				= vertexBindinginUse.bindingDescription;
	pipeline.vertexAttributeDesc			= vertexBindinginUse.attributeDescription;
	m_skyboxForwardPass->SetFrameBuffer(m_forwardPass->GetFrameBuffer());
	RETURN_FALSE_IF_FALSE(m_skyboxForwardPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryAndSceneLayout;
	pipeline.vertexInBinding				= vertexBindinginUse.bindingDescription;
	pipeline.vertexAttributeDesc			= vertexBindinginUse.attributeDescription;
	RETURN_FALSE_IF_FALSE(m_skyboxDeferredPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryAndSceneLayout;
	pipeline.vertexInBinding				= vertexBindinginUse.bindingDescription;
	pipeline.vertexAttributeDesc			= vertexBindinginUse.attributeDescription;
	RETURN_FALSE_IF_FALSE(m_deferredPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryLayout;
	RETURN_FALSE_IF_FALSE(m_ssaoComputePass->Initalize(nullptr, pipeline));
	RETURN_FALSE_IF_FALSE(m_ssaoBlurPass->Initalize(nullptr, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryAndSceneLayout;
	RETURN_FALSE_IF_FALSE(m_deferredLightPass->Initalize(nullptr, pipeline));
	RETURN_FALSE_IF_FALSE(m_ssrComputePass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= debugLayout;
	RETURN_FALSE_IF_FALSE(m_debugDrawPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryLayout;
	RETURN_FALSE_IF_FALSE(m_toneMapPass->Initalize(&renderData, pipeline));

	pipeline = CVulkanRHI::Pipeline{};
	pipeline.pipeLayout = primaryLayout;
	RETURN_FALSE_IF_FALSE(m_taaComputePass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= uiLayout;
	pipeline.cullMode						= VK_CULL_MODE_NONE;
	pipeline.enableBlending					= true;
	pipeline.enableDepthTest				= false;
	pipeline.enableDepthWrite				= false;
	RETURN_FALSE_IF_FALSE(m_uiPass->Initalize(&renderData, pipeline));

	return true;
}

void CRasterRender::UpdateCamera(CCamera::UpdateData& p_updateData)
{
	p_updateData.moveCamera							= m_keys[MIDDLE_MOUSE_BUTTON].down;
	p_updateData.mouseDelta[0]						= mouse_delta[0];
	p_updateData.mouseDelta[1]						= mouse_delta[1];
	p_updateData.A									= m_keys[(int)char('A')].down;
	p_updateData.S									= m_keys[(int)char('S')].down;
	p_updateData.D									= m_keys[(int)char('D')].down;
	p_updateData.W									= m_keys[(int)char('W')].down;
	p_updateData.Q									= m_keys[(int)char('Q')].down;
	p_updateData.E									= m_keys[(int)char('E')].down;
	p_updateData.Shft								= (m_keys[160].down || m_keys[16].down);
	p_updateData.jitter								= m_taaComputePass->GetJitterHelper()->ComputeJitter(m_frameCount);
	m_primaryCamera->Update(p_updateData);

	UpdateSceneGraphDependencies(p_updateData.timeDelta);
}

void CRasterRender::UpdateSceneGraphDependencies(float p_delta)
{
	// if the scene bounds have changed (this happens when an entity moves out of scene bounds in the previous frame) 
	// sunlight's camera is re-initialized with new scene bounding metrics. Their view and projection matrices are recalculated
	//if (m_sceneGraph->GetSceneStatus() == CSceneGraph::SceneStatus::ss_BoundsChange)
	//{
	//	// shadow camera by default will always look downwards, like sun at 12 o'clock
	//	const BBox* sceneBB = m_sceneGraph->GetBoundingBox();

	//	COrthoCamera::OrthInitdData initData{};
	//	initData.lookFrom = nm::float4(0.0f);
	//	initData.lrbt = nm::float4{ -sceneBB->GetWidth() / 2.0f, sceneBB->GetWidth() / 2.0f, -sceneBB->GetDepth() / 2.0f, sceneBB->GetDepth() / 2.0f };
	//	initData.nearPlane = 0;
	//	initData.farPlane = sceneBB->GetHeight();
	//	initData.yaw = 0.0f;
	//	initData.pitch = 0.0f;
	//	m_sunLight->Init(initData);

	//	nm::float3 sunLightDirection = m_sunLight->GetDirection();
	//	
	//	nm::Transform transform = m_sunLight->GetTransform();
	//	transform.SetRotation(atan2(sunLightDirection.x(), sunLightDirection.z()), asin(sunLightDirection.y()), 0.0f);
	//	transform.SetTranslate(nm::float4((sceneBB->bbMin[0] + sceneBB->bbMax[0]) / 2.0f, sceneBB->bbMax[1], (sceneBB->bbMin[2] + sceneBB->bbMax[2]) / 2.0f, 1.0f));
	//	m_sunLight->SetTransform(transform);
	//}

	//CCamera::UpdateData cdata;
	//cdata.timeDelta = p_delta;
	//m_sunLight->Update(cdata);
}

bool CRasterRender::DoReadBackObjPickerBuffer(uint32_t p_swapchainIndex, CVulkanRHI::CommandBuffer& p_cmdBfr)
{
	CVulkanRHI::Buffer buffer				= m_fixedAssets->GetFixedBuffers()->GetBuffer(CFixedBuffers::fb_ObjectPickerWrite);
	
	if(!m_rhi->BeginCommandBuffer(p_cmdBfr, "ReadBack Object Picker"))
		return false;

	// Barrier to ensure that shader writes are finished before buffer is read back from GPU
	m_rhi->IssueBufferBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, buffer.descInfo.buffer, p_cmdBfr);

	// Read back to host visible buffer
	VkBufferCopy copyRegion					= {};
	copyRegion.size							= buffer.descInfo.range;
	vkCmdCopyBuffer(p_cmdBfr, buffer.descInfo.buffer, buffer.descInfo.buffer, 1, &copyRegion);

	// Barrier to ensure that buffer copy is finished before host reading from it
	m_rhi->IssueBufferBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, buffer.descInfo.buffer, p_cmdBfr);

	m_rhi->EndCommandBuffer(p_cmdBfr);
	
	return true;
}

bool CRasterRender::RenderFrame(CVulkanRHI::RendererType p_renderType)
{
	CPass::RenderData renderData{};
	renderData.fixedAssets = m_fixedAssets;
	renderData.loadedAssets = m_loadableAssets;
	renderData.primaryDescriptors = m_primaryDescriptors;
	renderData.scIdx = m_swapchainIndex;
	renderData.sceneGraph = m_sceneGraph;

	//if (!m_staticShadowPass->ReuseShadowMap())
	{
		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_ShadowMap];
		RETURN_FALSE_IF_FALSE(m_staticShadowPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	if (p_renderType == CVulkanRHI::RendererType::Forward)
	{
		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Skybox];
		RETURN_FALSE_IF_FALSE(m_skyboxForwardPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);

		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Forward];
		RETURN_FALSE_IF_FALSE(m_forwardPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}
	else
	{
		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Skybox];
		RETURN_FALSE_IF_FALSE(m_skyboxDeferredPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);

		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Deferred_GBuf];
		RETURN_FALSE_IF_FALSE(m_deferredPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSAO];
	RETURN_FALSE_IF_FALSE(m_ssaoComputePass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSAO_Blur];
	RETURN_FALSE_IF_FALSE(m_ssaoBlurPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	if (p_renderType == CVulkanRHI::RendererType::Deferred)
	{
		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Deferred_Lighting];
		RETURN_FALSE_IF_FALSE(m_deferredLightPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);

		if (m_ssrComputePass->IsEnabled())
		{
			renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSR];
			RETURN_FALSE_IF_FALSE(m_ssrComputePass->Render(&renderData));
			m_cmdBfrsInUse.push_back(renderData.cmdBfr);
		}
	}

	// placing debug draw here because I do not want the depth buffer to go through another layout barrier
	// from depth stencil attachment to shader read used in SSAO pass
	renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_DebugDraw];
	if (m_debugDrawPass->Render(&renderData))
	{
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	if (m_taaComputePass->IsEnabled())
	{
		renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_TAA];
		RETURN_FALSE_IF_FALSE(m_taaComputePass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_ToneMapping];
	RETURN_FALSE_IF_FALSE(m_toneMapPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr = m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_UI];
	RETURN_FALSE_IF_FALSE(m_uiPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	return false;
}

// Placing the notes here until I can find the best way to place them in my read me
/*
Ortho Camera
initialization:
	1. Calculate yaw, pitch and view projection matrix
	2. Create bounding box of camera with min.z = 0 && max.z = 1
	3. Convert this bbox from View space to workd space by multiplying with inverse view proj matrix
	4. Set it as the bounding box
	5. Update the associated tranaltion of the entity based on look from value
	6. Update the accociated roation of the entity based on yaw and pitch values

Update:
When Scene bounds dont change
	1. If the transform has been changed, it is labled as dirty
	2. Update look from, yaw and pitch from trnsform
	3. Recalculate the view and projection matrix
	4. Eecalculate the Up vector

When Scene bounds change
	1. The camera is re-initialized with new scene bounding metrics. Their view and projection matrices are recalculated
	2. The transform of the camera is rotated to reflect the way we want the sunlight to be initialized to; seeing downwards
	3. Since the bbox is not cenetered in the middle but offset like a Camera's bbox, an offset tranaltion of half of the size of the scene is applied for correct for the translation caused during the rotation


When do Scene bounds change state ?
	1. When an entity's bounding box changes; a request to scene bounding box is made
	2. During the next frame's update process; the scene graph is updated
	3. Duing the scene graph's update process, the scene status is always set to SceneStatus::ss_NoChange by default
	4. If there is a request to update scene bounding box pending from previous frame, the current bounding box is cached to temp and discarded
	5. The entire scene graph is traversed and the new bounds are calculated. The scene status is set to SceneStatus::ss_SceneMoved now; so this can be used to recompute shadow maps
	6. If the new scene bounds are different from the previous frame's bouds, the scene status is set to SceneStatus::ss_BoundsChange; so the shaodw camera can be re-created

When are shadow maps re-computed
	1. When the scene state is not SceneStatus::ss_NoChange	ie: when something in the world moves or
	2. When the shadow camera moves
*/