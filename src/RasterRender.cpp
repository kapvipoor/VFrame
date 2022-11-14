#pragma once

#include "core/RandGen.h"
#include "RasterRender.h"
#include "external/imgui/imgui.h"
#include <assert.h>

//float g_sunDistanceFromOrigin = 50.0f;
//nm::float4 g_sunDirection = nm::float4(0.0f, 1.0f, 0.0f, 0.0f);// -93.83f);

CRasterRender::CRasterRender(const char* name, int screen_width_, int screen_height_, int window_scale)
		:CWinCore(name, screen_width_, screen_height_, window_scale)
		, m_pickObject(false)

{			
	m_rhi					= new CVulkanRHI(name, screen_width_, screen_height_);

	m_sceneGraph			= new CSceneGraph();

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
	m_debugDrawPass			= new CDebugDrawPass(m_rhi);
	m_toneMapPass			= new CToneMapPass(m_rhi);
	m_uiPass				= new CUIPass(m_rhi);

	m_primaryCamera			= new CPerspectiveCamera("Primary Camera");
	m_sunLightCamera		= new COrthoCamera("Sunlight Camera");
}

CRasterRender::~CRasterRender() 
{
	delete m_sunLightCamera;
	delete m_primaryCamera;

	delete m_sceneGraph;

	delete m_uiPass;
	delete m_toneMapPass;
	delete m_debugDrawPass;
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
	m_toneMapPass->Destroy();
	m_debugDrawPass->Destroy();
	m_ssaoBlurPass->Destroy();
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
	// (might be gfx compatible as well)
	RETURN_FALSE_IF_FALSE(m_rhi->CreateCommandPool(m_rhi->GetQueueFamiliyIndex(), m_vkCmdPool));

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		RETURN_FALSE_IF_FALSE(m_rhi->CreateCommandBuffers(m_vkCmdPool, m_vkCmdBfr[i], CommandBufferId::cb_max));
	}

	RETURN_FALSE_IF_FALSE(CreateSyncPremitives());

	RETURN_FALSE_IF_FALSE(m_fixedAssets->Create(m_rhi));

	RETURN_FALSE_IF_FALSE(m_loadableAssets->Create(m_rhi, *m_fixedAssets, m_vkCmdPool));
	
	RETURN_FALSE_IF_FALSE(m_primaryDescriptors->Create(m_rhi, *m_fixedAssets, *m_loadableAssets));

	RETURN_FALSE_IF_FALSE(CreatePasses());

	{
		RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(m_vkCmdBfr[0][0], "Init Barriers"));

		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_SSAO);
		m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, m_vkCmdBfr[0][0], CRenderTargets::rt_SSAOBlur);
		
		//m_fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_vkCmdBfr[0][0], CRenderTargets::rt_DeferredLighting);

		RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(m_vkCmdBfr[0][0]));

		RETURN_FALSE_IF_FALSE(m_rhi->ResetFence(m_vkFenceCmdBfrFree[0]));

		VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		VkSubmitInfo submitInfo{};
		VkQueue cmdQueue							= m_rhi->GetQueue(0);
		submitInfo.sType							= VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount				= 1;
		submitInfo.pCommandBuffers					= &m_vkCmdBfr[0][0];
		submitInfo.pSignalSemaphores				= VK_NULL_HANDLE;
		submitInfo.signalSemaphoreCount				= 0;
		submitInfo.pWaitSemaphores					= VK_NULL_HANDLE;
		submitInfo.waitSemaphoreCount				= 0;
		submitInfo.pWaitDstStageMask				= &waitstage;
		RETURN_FALSE_IF_FALSE(m_rhi->SubmitCommandbuffer(cmdQueue, &submitInfo, 1, m_vkFenceCmdBfrFree[0]));
	}

	RETURN_FALSE_IF_FALSE(InitCamera());

	return true;
}

bool CRasterRender::on_update(float delta)
{
	RETURN_FALSE_IF_FALSE(m_rhi->AcquireNextSwapChain(m_vkswapchainAcquireSemaphore, m_swapchainIndex));

	m_sceneGraph->Update();
	UpdateCamera(delta);

	// if left mouse is clicked; prepare and pick object for this frame
	m_pickObject = false;  m_keys[LEFT_MOUSE_BUTTON].down;

	int mousepos_x = 0, mousepos_y = 0;
	GetCurrentMousePosition(mousepos_x, mousepos_y);
	
	{
		CFixedBuffers::PrimaryUniformData uniformData = m_fixedAssets->GetFixedBuffers()->GetPrimaryUnifromData();
		uniformData.elapsedTime						= delta;
		uniformData.cameraInvView					= nm::inverse(m_primaryCamera->GetView());
		uniformData.cameraLookFrom					= m_primaryCamera->GetLookFrom();
		uniformData.cameraProj						= m_primaryCamera->GetProjection();
		uniformData.cameraView						= m_primaryCamera->GetView();
		uniformData.cameraViewProj					= m_primaryCamera->GetViewProj();
		uniformData.mousePos						= nm::float2((float)mousepos_x, (float)mousepos_y);
		uniformData.renderRes						= nm::float2((float)m_rhi->GetRenderWidth(), (float)m_rhi->GetRenderHeight());
		uniformData.skyboxModelView					= m_primaryCamera->GetView();
		uniformData.sunDirViewSpace					= (m_primaryCamera->GetView() * nm::float4(m_sunLightCamera->GetLookAt(), 1.0f)).xyz();
		uniformData.sunDirWorldSpace				= m_sunLightCamera->GetLookAt();
		uniformData.sunViewProj						= m_sunLightCamera->GetViewProj();
		//uniformData.sunIntensity					= 10.0f; // no need for update, this is updated from the user interface

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
		m_forwardPass->Update(&updateData);
		m_deferredPass->Update(&updateData);
		m_deferredLightPass->Update(&updateData);
		m_debugDrawPass->Update(&updateData);
		m_toneMapPass->Update(&updateData);
		m_uiPass->Update(&updateData);
	}

	if (!m_rhi->WaitFence(m_vkFenceCmdBfrFree[m_swapchainIndex]))
		return false;

	if (!m_rhi->ResetFence(m_vkFenceCmdBfrFree[m_swapchainIndex]))
		return false;

	if (m_rhi->GetRendererType() == CVulkanRHI::RendererType::Forward)
	{
		RenderForward();
	}
	else
	{
		RenderDeferred();
	}

	if (m_pickObject)
	{
		m_cmdBfrsInUse.push_back(m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_PickerCopy2CPU]);
		if (!DoReadBackObjPickerBuffer(m_swapchainIndex, m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_PickerCopy2CPU]))
			return false;
	}

	CVulkanRHI::Queue queue						= m_rhi->GetQueue(0);
	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType							= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount				= (uint32_t)m_cmdBfrsInUse.size();
	submitInfo.pCommandBuffers					= m_cmdBfrsInUse.data();
	submitInfo.pSignalSemaphores				= &m_vksubmitCompleteSemaphore;
	submitInfo.signalSemaphoreCount				= 1;
	submitInfo.pWaitSemaphores					= &m_vkswapchainAcquireSemaphore;
	submitInfo.waitSemaphoreCount				= 1;
	submitInfo.pWaitDstStageMask				= &waitstage;
	RETURN_FALSE_IF_FALSE(m_rhi->SubmitCommandbuffer(queue, &submitInfo, 1, m_vkFenceCmdBfrFree[m_swapchainIndex]));
	m_cmdBfrsInUse.clear();

	if (m_pickObject)
	{
		int meshID = -1;
		RETURN_FALSE_IF_FALSE(m_rhi->ReadFromBuffer((uint8_t*)&meshID, m_fixedAssets->GetFixedBuffers()->GetBuffer(CFixedBuffers::fb_ObjectPickerRead)));
		m_loadableAssets->GetScene()->SetSelectedRenderableMesh(meshID);
		std::cout << "Picked Mesh: " << (meshID - 1) << std::endl;
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
	presentInfo.swapchainCount					= 1;										// number of swap chains we want to present to
	presentInfo.pSwapchains						= &swapchain;
	presentInfo.pImageIndices					= &m_swapchainIndex;
	presentInfo.pResults						= &presentResult;
	VkResult res = vkQueuePresentKHR(queue, &presentInfo);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkQueueSubmit failed " << res << std::endl;
	}
}

bool CRasterRender::InitCamera()
{
	CPerspectiveCamera::PerpspectiveInitdData persIntData{};
	persIntData.fov								= 45.0f;
	persIntData.aspect							= (float)CWinCore::s_Window.screenWidth / CWinCore::s_Window.screenHeight;
	persIntData.lookFrom						= nm::float4(0.0f, 50.0f, 0.0f, 1.0f);
	persIntData.lookAt							= nm::float3{ 0.0f, 0.0f, 0.0f };
	persIntData.up								= nm::float3{ 0.0f, 1.0f,  0.0f };
	persIntData.aperture						= 0.1f;							
	persIntData.focusDistance					= 10.0f;							
	persIntData.yaw								= 0.0f;									
	persIntData.pitch							= 0.0f;							
	RETURN_FALSE_IF_FALSE( m_primaryCamera->Init(&persIntData));

	COrthoCamera::OrthInitdData orthoInit{};
	orthoInit.lookFrom							= nm::float4(0.0f);
	orthoInit.lrbt								= nm::float4{ -50.0f, 50.0f, -50.0f, 50.0f };
	orthoInit.nearPlane							= 0;
	orthoInit.farPlane							= 50;
	orthoInit.yaw								= 0.0f;
	orthoInit.pitch								= 0.0f;
	RETURN_FALSE_IF_FALSE(m_sunLightCamera->Init(&orthoInit));

	return true;
}

bool CRasterRender::CreateSyncPremitives()
{
	RETURN_FALSE_IF_FALSE(m_rhi->CreateSemaphor(m_vkswapchainAcquireSemaphore));

	RETURN_FALSE_IF_FALSE(m_rhi->CreateSemaphor(m_vksubmitCompleteSemaphore));

	RETURN_FALSE_IF_FALSE(m_rhi->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT, m_vkFenceCmdBfrFree[0]));

	RETURN_FALSE_IF_FALSE(m_rhi->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT, m_vkFenceCmdBfrFree[1]));

	return true;
}

void CRasterRender::DestroySyncPremitives()
{
	m_rhi->DestroySemaphore(m_vkswapchainAcquireSemaphore);
	m_rhi->DestroySemaphore(m_vksubmitCompleteSemaphore);
	m_rhi->DestroyFence(m_vkFenceCmdBfrFree[0]);
	m_rhi->DestroyFence(m_vkFenceCmdBfrFree[1]);
}

bool CRasterRender::CreatePasses()
{
	// Push Constants for G Buffers
	VkPushConstantRange meshPushrange{};
	meshPushrange.offset					= 0;
	meshPushrange.size						= sizeof(CScene::MeshPushConst);	// pushing mesh and submesh ID
	meshPushrange.stageFlags				= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	// Push constants for UI
	VkPushConstantRange uiPushrange{};
	uiPushrange.offset						= 0;
	uiPushrange.size						= sizeof(CRenderableUI::UIPushConstant);
	uiPushrange.stageFlags					= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayout primaryAndSceneLayout;
	std::vector<VkDescriptorSetLayout> descLayouts;
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_loadableAssets->GetScene()->GetDescriptorSetLayout());			// Set 1 - BindingSet::Mesh
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&meshPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), primaryAndSceneLayout));

	VkPipelineLayout primaryLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&meshPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), primaryLayout));

	VkPipelineLayout uiLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_loadableAssets->GetUI()->GetDescriptorSetLayout());				// Set 1 - BindingSet::UI
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(&uiPushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), uiLayout));

	VkPipelineLayout debugLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescriptors->GetDescriptorSetLayout(0));					// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_fixedAssets->GetDebugRenderer()->GetDescriptorSetLayout());		// Set 1 - BindingSet::Debug
	RETURN_FALSE_IF_FALSE(m_rhi->CreatePipelineLayout(nullptr, 0, descLayouts.data(), (uint32_t)descLayouts.size(), debugLayout));


	CPass::RenderData renderData{};
	renderData.fixedAssets					= m_fixedAssets;
	renderData.loadedAssets					= m_loadableAssets;
	renderData.rendererType					= m_rhi->GetRendererType();

	CVulkanRHI::Pipeline pipeline;
	pipeline.pipeLayout						= primaryAndSceneLayout;
	pipeline.cullMode						= VK_CULL_MODE_BACK_BIT;		// to fix peterpanning issue
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
	pipeline.renderpassData					= m_forwardPass->GetRenderpass();			// reusing renderpass from forward pass
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
	RETURN_FALSE_IF_FALSE(m_deferredLightPass->Initalize(nullptr, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= debugLayout;
	RETURN_FALSE_IF_FALSE(m_debugDrawPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= primaryLayout;
	RETURN_FALSE_IF_FALSE(m_toneMapPass->Initalize(&renderData, pipeline));

	pipeline								= CVulkanRHI::Pipeline{};
	pipeline.pipeLayout						= uiLayout;
	pipeline.cullMode						= VK_CULL_MODE_NONE;
	pipeline.enableBlending					= true;
	pipeline.enableDepthTest				= false;
	pipeline.enableDepthWrite				= false;
	//RETURN_FALSE_IF_FALSE(m_uiForwardPass->Initalize(&renderData, pipeline));
	//RETURN_FALSE_IF_FALSE(m_uiDeferredPass->Initalize(&renderData, pipeline));
	RETURN_FALSE_IF_FALSE(m_uiPass->Initalize(&renderData, pipeline));

	return true;
}

bool CRasterRender::UpdateCamera(float p_delta)
{
	CCamera::UpdateData cdata{};
	cdata.moveCamera						= m_keys[MIDDLE_MOUSE_BUTTON].down;
	cdata.timeDelta							= p_delta;
	cdata.mouseDelta[0]						= mouse_delta[0];
	cdata.mouseDelta[1]						= mouse_delta[1];
	cdata.A									= m_keys[(int)char('A')].down;
	cdata.S									= m_keys[(int)char('S')].down;
	cdata.D									= m_keys[(int)char('D')].down;
	cdata.W									= m_keys[(int)char('W')].down;
	cdata.Q									= m_keys[(int)char('Q')].down;
	cdata.E									= m_keys[(int)char('E')].down;
	cdata.Shft								= (m_keys[160].down || m_keys[16].down);
	m_primaryCamera->Update(cdata);

	// if the scene bounds have changed, reinitialize the sunlight camera
	if (m_sceneGraph->GetSceneStatus() == CSceneGraph::SceneStatus::ss_BoundsChange)
	{
		// shadow camera by default will always look downwards, like sun at 12 o'clock
		const BBox* sceneBB					= m_sceneGraph->GetBoundingBox();
		
		COrthoCamera::OrthInitdData orthoInit{};
		orthoInit.lookFrom = nm::float4(0.0f);
		orthoInit.lrbt						= nm::float4{ -sceneBB->GetWidth()/2.0f, sceneBB->GetWidth() / 2.0f, -sceneBB->GetDepth() / 2.0f, sceneBB->GetDepth() / 2.0f };
		orthoInit.nearPlane					= 0;
		orthoInit.farPlane					= sceneBB->GetHeight();
		orthoInit.yaw						= 0.0f;
		orthoInit.pitch						= 0.0f;
		RETURN_FALSE_IF_FALSE(m_sunLightCamera->Init(&orthoInit));

		nm::Transform transform = m_sunLightCamera->GetTransform();
		transform.SetRotation(0.0f, 1.57f, 0.0f);		// looking straight down on the scene
		transform.SetTranslate(nm::float4((sceneBB->bbMin[0] + sceneBB->bbMax[0]) / 2.0f, sceneBB->bbMax[1], (sceneBB->bbMin[2] + sceneBB->bbMax[2]) / 2.0f, 1.0f));
		m_sunLightCamera->SetTransform(transform);
	}

	cdata									= CCamera::UpdateData();
	cdata.timeDelta							= p_delta;
	m_sunLightCamera->Update(cdata);

	return true;
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

bool CRasterRender::RenderDeferred()
{
	CPass::RenderData renderData{};
	renderData.fixedAssets					= m_fixedAssets;
	renderData.loadedAssets					= m_loadableAssets;
	renderData.primaryDescriptors			= m_primaryDescriptors;
	renderData.scIdx						= m_swapchainIndex;
	renderData.sceneGraph					= m_sceneGraph;
	renderData.rendererType					= m_rhi->GetRendererType();

	if (!m_staticShadowPass->ReuseShadowMap())
	{
		renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_ShadowMap];
		RETURN_FALSE_IF_FALSE(m_staticShadowPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Skybox];
	RETURN_FALSE_IF_FALSE(m_skyboxDeferredPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Deferred_GBuf];
	RETURN_FALSE_IF_FALSE(m_deferredPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSAO];
	RETURN_FALSE_IF_FALSE(m_ssaoComputePass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSAO_Blur];
	RETURN_FALSE_IF_FALSE(m_ssaoBlurPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Deferred_Lighting];
	RETURN_FALSE_IF_FALSE(m_deferredLightPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	// placing debug draw here because I do not want the depth buffer to go through another layout barrier
	// from depth stencil attachment to shader read used in ssao pass
	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_DebugDraw];
	if (m_debugDrawPass->Render(&renderData))
	{
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_ToneMapping];
	RETURN_FALSE_IF_FALSE(m_toneMapPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr						= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_UI];
	RETURN_FALSE_IF_FALSE(m_uiPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	return true;
}

bool CRasterRender::RenderForward()
{
	CPass::RenderData renderData{};
	renderData.fixedAssets				= m_fixedAssets;
	renderData.loadedAssets				= m_loadableAssets;
	renderData.primaryDescriptors		= m_primaryDescriptors;
	renderData.scIdx					= m_swapchainIndex;
	renderData.sceneGraph				= m_sceneGraph;

	if (!m_staticShadowPass->ReuseShadowMap())
	{
		renderData.cmdBfr				= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_ShadowMap];
		RETURN_FALSE_IF_FALSE(m_staticShadowPass->Render(&renderData));
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Skybox];
	RETURN_FALSE_IF_FALSE(m_skyboxForwardPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_Forward];
	RETURN_FALSE_IF_FALSE(m_forwardPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSAO];
	RETURN_FALSE_IF_FALSE(m_ssaoComputePass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_SSAO_Blur];
	RETURN_FALSE_IF_FALSE(m_ssaoBlurPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	// placing debug draw here because I do not want the depth buffer to go through another layout barrier
	// from depth stencil attachment to shader read used in ssao pass
	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_DebugDraw];
	if (m_debugDrawPass->Render(&renderData))
	{
		m_cmdBfrsInUse.push_back(renderData.cmdBfr);
	}

	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_ToneMapping];
	RETURN_FALSE_IF_FALSE(m_toneMapPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	renderData.cmdBfr					= m_vkCmdBfr[m_swapchainIndex][CommandBufferId::cb_UI];
	RETURN_FALSE_IF_FALSE(m_uiPass->Render(&renderData));
	m_cmdBfrsInUse.push_back(renderData.cmdBfr);

	return true;
}
