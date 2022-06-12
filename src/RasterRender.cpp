#pragma once

#include "core/RandGen.h"
#include "RasterRender.h"

float g_sunDistanceFromOrigin = 50.5f;
nm::float4 g_sunPosition = nm::float4(0.0f, 1.0f, 0.0f, 0.0f);// -93.83f);

CRasterRender::CRasterRender(const char* name, int screen_width_, int screen_height_, int window_scale)
		:CWinCore(name, screen_width_, screen_height_, window_scale)
		, CVulkanCore(name, screen_width_, screen_height_)
		, m_pickObject(false)
		, m_primaryCamera(
			  45.0f, (float)screen_width_ / screen_height_
			, (g_sunDistanceFromOrigin* g_sunPosition).xyz()	// lookFrom
			, nm::float3{ 0.0f, 0.0f, 0.0f }					// lookAt
			, nm::float3{ 0.0f, 1.0f,  0.0f }					// up
			, 0.1f												// aperture
			, 10.0f												// focus distance
			, 0.0f												// yaw
			, 1.5708f)											// pitch
		
		, m_sunLightCamera(
			  g_sunDistanceFromOrigin * g_sunPosition			// lookFrom
			, g_sunPosition.xyz()								// lookAt
			, nm::float3{ 0.0f, 1.0f, 0.0f }					// up
			, nm::float4{ -g_sunDistanceFromOrigin, g_sunDistanceFromOrigin, -g_sunDistanceFromOrigin, g_sunDistanceFromOrigin }		// lrbt
			, -g_sunDistanceFromOrigin											// near
			, 250												// far
			, 0.0f/*1.579f*/									// yaw
			, 1.5708f/*4.71239f*//*0.714f*/)									// pitch

		//, m_sunLightCamera(
		//	  45.0f, (float)screen_width_ / screen_height_
		//	, (g_sunDistanceFromOrigin * g_sunPosition).xyz()
		//	, nm::float3{ 0.0f, 0.0f, 0.0f }					// lookAt			- does not matter for Light Camera
		//	, nm::float3{ 0.0f, 0.0f,  1.0f }					// up				- does not matter for Light Camera
		//	, 0.1f												// aperture			- does not matter for Light Camera
		//	, 10.0f												// focus distance	- does not matter for Light Camwera
		//	, 0.0f												// yaw
		//	, 1.5708f)											// pitch
{			
	m_fixedBufferList.resize(FixedBufferId::fb_max);
	m_renderTargets.resize(RenderTargetId::rt_max);
	m_samplers.resize(SamplerId::s_max);
}

CRasterRender::~CRasterRender() {};

void CRasterRender::on_destroy()
{

	ClearFixedResource();
		
	DestroyFence(m_vkFenceCmdBfrFree[0]);
	DestroyFence(m_vkFenceCmdBfrFree[1]);
	DestroySemaphore(m_vksubmitCompleteSemaphore);
	DestroySemaphore(m_vkswapchainAcquireSemaphore);

	// no need to explicity destroy the buffer, destroying the pool will do the job
	DestroyCommandPool(m_vkCmdPool);

	DestroyPipeline(m_forwardPipeline.pipeline);
	DestroyPipelineLayout(m_forwardPipeline.pipeLayout);

	DestroyPipeline(m_skyboxPipeline.pipeline);
	DestroyPipelineLayout(m_skyboxPipeline.pipeLayout);
	
	DestroyRenderpass(m_forwardRenderpass.renderpass);

	DestroyDescriptorSetLayout(m_primaryDescLayout[0]);
	DestroyDescriptorSetLayout(m_primaryDescLayout[1]);
	// No need to explecitly destroy the descriptors, destroying the descriptors pool will 
	// destroy all the allocated descriptors
	DestroyDescriptorPool(m_primaryDescPool);

	cleanUp();
}

bool CRasterRender::on_create(HINSTANCE pInstance)
{
	VkInitData initData{};
	initData.winInstance = pInstance;
	initData.winHandle = m_hwnd;
	initData.queueType = VK_QUEUE_GRAPHICS_BIT;
	initData.swapChaineImageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	initData.swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	if (!CVulkanCore::initialize(initData))
		return false;

	// Will create command pool for Compute Queue Family only 
	// (might be gfx compatible as well)
	if (!CreateCommandPool(m_QFIndex, m_vkCmdPool))
		return false;

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		if (!CreateCommandBuffers(m_vkCmdPool, m_vkCmdBfr[i], CommandBufferId::cb_max))
			return false;
	}

	if (!CreateFixedResources())
		return false;

	if (!CreateReadOnlyResources())
		return false;

	SetLayoutForDescriptorCreaton();

	if (!CreatePrimaryDescriptors())
		return false;

	UnsetLayoutForDescriptorCreaton();

	if (!CreateSyncPremitives())
		return false;

	// testing models from https://casual-effects.com/data/
	std::string paths[]{
		//(g_AssetPath + "/glTFSampleModels/2.0/DragonAttenuation/glTF/DragonAttenuation.gltf")
		//(g_AssetPath + "/shadow_test_3.gltf")
		//(g_AssetPath + "/glTFSampleModels/2.0/TransmissionTest/glTF/TransmissionTest.gltf")
		//"D:/Vipul Kapoor/RTinAWeekend/assets/glTFSampleModels/2.0/NormalTangentMirrorTest/glTF/NormalTangentMirrorTest.gltf"
		//,"D:/Vipul Kapoor/RTinAWeekend/assets/glTFSampleModels/2.0/Suzanne/glTF/Suzanne.gltf"
		(g_AssetPath + "/Sponza/glTF/Sponza.gltf")
		//"D:/Vipul Kapoor/RTinAWeekend/assets/DamagedHelmet/glTF/DamagedHelmet.gltf"
		//"D:/Vipul Kapoor/RTinAWeekend/assets/cube/cube.obj"
	};
	bool flipYList[]{ false, false, false};
	if (!CreateScene(paths, flipYList, 1))
		return false;

	if (!CreateForwardRenderpass())
		return false;

	if (!CreateDeferredRenderpass())
		return false;

	if (!CreateLightDepthRenderpass())
		return false;

	if (!CreatePipelines())
		return false;

	{
		if (!BeginCommandBuffer(m_vkCmdBfr[0][0], "Init Barriers"))
			return false;

		IssueLayoutBarrier(VK_IMAGE_LAYOUT_GENERAL, m_renderTargets[rt_SSAO], m_vkCmdBfr[0][0]);
		IssueLayoutBarrier(VK_IMAGE_LAYOUT_GENERAL, m_renderTargets[rt_SSAOBlur], m_vkCmdBfr[0][0]);
		IssueLayoutBarrier(VK_IMAGE_LAYOUT_GENERAL, m_renderTargets[rt_DeferredLighting], m_vkCmdBfr[0][0]);

		if (!EndCommandBuffer(m_vkCmdBfr[0][0]))
			return false;

		if (!ResetFence(m_vkFenceCmdBfrFree[0]))
			return false;

		VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_vkCmdBfr[0][0];
		submitInfo.pSignalSemaphores = VK_NULL_HANDLE;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = VK_NULL_HANDLE;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitDstStageMask = &waitstage;
		if (!SubmitCommandbuffer(m_vkQueue[0], &submitInfo, 1, m_vkFenceCmdBfrFree[0]))
			return false;
	}

	return true;
}

std::vector<VkCommandBuffer> cmdBfrsInUse;

bool CRasterRender::on_update(float delta)
{
	uint32_t curSCIndex = 0;
	if (!AcquireNextSwapChain(m_vkswapchainAcquireSemaphore, curSCIndex))
		return false;

	// if left mouse is clicked; prepare and pick object for this frame
	m_pickObject = m_keys[LEFT_MOUSE_BUTTON].down;

	UpdateScene(curSCIndex, delta);

	UpdateCamera(delta);

	if (!UpdateUniform(curSCIndex, delta))
		return false;

	if (!WaitFence(m_vkFenceCmdBfrFree[curSCIndex]))
		return false;

	if (!ResetFence(m_vkFenceCmdBfrFree[curSCIndex]))
		return false;

	if (!DoLightDepthPrepass(curSCIndex))
		return false;
	
	if (!DoSkybox(curSCIndex))
		return false;

	if (!DoGBuffer_Forward(curSCIndex))
		return false;
	
	if (!DoGBuffer_Deferred(curSCIndex))
		return false;

	if (!DoSSAO(curSCIndex))
		return false;

	if (!DoDeferredLighting(curSCIndex))
		return false;

	if (m_pickObject)
	{
		if (!DoReadBackObjPickerBuffer(curSCIndex))
			return false;
	}

	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount	= (uint32_t)cmdBfrsInUse.size();
	submitInfo.pCommandBuffers		= cmdBfrsInUse.data();
	submitInfo.pSignalSemaphores	= &m_vksubmitCompleteSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pWaitSemaphores		= &m_vkswapchainAcquireSemaphore;
	submitInfo.waitSemaphoreCount	= 1;
	submitInfo.pWaitDstStageMask	= &waitstage;
	if (!SubmitCommandbuffer(m_vkQueue[0], &submitInfo, 1, m_vkFenceCmdBfrFree[curSCIndex]))
		return false;

	// as of now only using it for presentaiton
	m_swapchainIndex = curSCIndex;

	if (m_pickObject)
	{
		uint32_t meshID = -1;
		if (!DownloadDataFromBuffer((uint8_t*)&meshID, m_fixedBufferList[fb_ObjectPickerRead]))
			return false;
		std::cout << "Picked Mesh: " << (meshID - 1) << std::endl;
	}

	cmdBfrsInUse.clear();

	return true;
}

void CRasterRender::on_present()
{
	VkResult presentResult			= VkResult::VK_RESULT_MAX_ENUM;
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount	= 1;
	presentInfo.pWaitSemaphores		= &m_vksubmitCompleteSemaphore;
	presentInfo.swapchainCount		= 1;										// number of swap chains we want to present to
	presentInfo.pSwapchains			= &m_vkSwapchain;
	presentInfo.pImageIndices		= &m_swapchainIndex;
	presentInfo.pResults			= &presentResult;
	VkResult res = vkQueuePresentKHR(m_vkQueue[0], &presentInfo);
	if (res != VK_SUCCESS)
	{
		std::cerr << "vkQueueSubmit failed " << res << std::endl;
	}
}

bool CRasterRender::UpdateScene(uint32_t p_swapchainIndex, float p_elapsedDelta)
{
	for (auto& mesh : m_scene.meshes)
	{
		//mesh.modelMatrix = nm::float4x4().identity();
		mesh.viewNormalTransform = nm::transpose(nm::inverse(m_primaryCamera.GetView() * mesh.modelMatrix));
	}

	return true;
}

bool CRasterRender::UpdateUniform(uint32_t p_swapchainIndex, float p_elapsedDelta)
{
	// update primary uniforms
	{
		int mousePos[]{ -10, -10 };
		if (m_pickObject)
		{
			GetCurrentMousePosition(mousePos[0], mousePos[1]);
		}

		// calculate transpose inverse 

		nm::float3 camLookFrom = m_primaryCamera.GetLookFrom();
		const float* camViewProj = &m_primaryCamera.GetViewProj().column[0][0];
		const float* camProj = &m_primaryCamera.GetProjection().column[0][0];
		const float* camView = &m_primaryCamera.GetView().column[0][0];
		const float* inv_camView = &nm::inverse(m_primaryCamera.GetView()).column[0][0];
		const float* sunLightViewProj = &m_sunLightCamera.GetViewProj().column[0][0];
		nm::float3 sunLightDirWorld = m_sunLightCamera.GetLookAt();
		nm::float3 sunLightDirView = (m_primaryCamera.GetView() * nm::float4(sunLightDirWorld, 1.0f)).xyz();

		float* skyboxView = const_cast<float*>(&m_primaryCamera.GetView().column[0][0]);
		// cancelling out translation for skybox rendering
		skyboxView[12] = 0.0;
		skyboxView[13] = 0.0;
		skyboxView[14] = 0.0;
		skyboxView[15] = 1.0;

		float ssaoNoiseTexDim = sqrtf((float)m_ssaoNoiseDimSquared);

		std::vector<float> uniformValues;
		uniformValues.push_back(p_elapsedDelta);																							// time elapsed delata
		std::copy(&camLookFrom[0], &camLookFrom[3], std::back_inserter(uniformValues));														// camera look from x, y, z
		std::copy(&camViewProj[0], &camViewProj[16], std::back_inserter(uniformValues));													// camera view projection matrix
		std::copy(&camProj[0], &camProj[16], std::back_inserter(uniformValues));															// camera projection matrix
		std::copy(&camView[0], &camView[16], std::back_inserter(uniformValues));															// camera view matrix
		std::copy(&inv_camView[0], &inv_camView[16], std::back_inserter(uniformValues));													// inverse camera view matrix
		std::copy(&skyboxView[0], &skyboxView[16], std::back_inserter(uniformValues));														// skybox model view
		uniformValues.push_back((float)m_renderWidth);	uniformValues.push_back((float)m_renderHeight);										// render resolution
		uniformValues.push_back((float)mousePos[0]);	uniformValues.push_back((float)mousePos[1]);										// mouse pos
		uniformValues.push_back((float)m_renderWidth / ssaoNoiseTexDim); uniformValues.push_back((float)m_renderHeight / ssaoNoiseTexDim);	// ssao noise scale
		uniformValues.push_back((float)m_ssaokernelSize);																					// ssao kernel size
		uniformValues.push_back((float)m_ssaoRadius);																						// ssao radius
		std::copy(&sunLightViewProj[0], &sunLightViewProj[16], std::back_inserter(uniformValues));											// sun light view projection matrix
		std::copy(&sunLightDirWorld[0], &sunLightDirWorld[3], std::back_inserter(uniformValues));											// sun light sunLightDirection x, y, z in world space
		uniformValues.push_back((float)0.0f);																								// unassigned_0
		std::copy(&sunLightDirView[0], &sunLightDirView[3], std::back_inserter(uniformValues));												// sun light sunLightDirection x, y, z in view space
		uniformValues.push_back((float)0.0f);																								// unassigned_1

		uint8_t* data = (uint8_t*)(uniformValues.data());

		uint32_t id = (p_swapchainIndex == 0) ? fb_PrimaryUniform_0 : fb_PrimaryUniform_1;
		if (!UploadDataToBuffer(data, m_fixedBufferList[id]))
			return false;
	}

	// update Mesh Uniform
	{
		std::vector<float> perMeshUniformData;
		for (const auto& mesh : m_scene.meshes)
		{
			const float* modelMat = &mesh.modelMatrix.column[0][0];
			const float* trn_inv_model = &nm::transpose(nm::inverse(mesh.modelMatrix)).column[0][0];

			std::copy(&modelMat[0], &modelMat[16], std::back_inserter(perMeshUniformData));					// model matrix for this mesh
			std::copy(&trn_inv_model[0], &trn_inv_model[16], std::back_inserter(perMeshUniformData));		// Tranpose(inverse(model)) for transforming normal to world space
		}

		uint8_t* data = (uint8_t*)(perMeshUniformData.data());

		if (!UploadDataToBuffer(data, m_scene.meshInfo_uniform))
			return false;
	}

	return true;
}

void CRasterRender::UpdateCamera(float p_delta)
{
	CCamera::CamData cdata{};
	cdata.moveCamera = m_keys[MIDDLE_MOUSE_BUTTON].down;
	cdata.timeDelta = p_delta;
	cdata.mouseDelta[0] = mouse_delta[0];
	cdata.mouseDelta[1] = mouse_delta[1];
	cdata.A = m_keys[(int)char('A')].down;
	cdata.S = m_keys[(int)char('S')].down;
	cdata.D = m_keys[(int)char('D')].down;
	cdata.W = m_keys[(int)char('W')].down;
	cdata.Q = m_keys[(int)char('Q')].down;
	cdata.E = m_keys[(int)char('E')].down;
	cdata.Shft = (m_keys[160].down || m_keys[16].down);
	m_primaryCamera.Update(cdata);

	cdata = CCamera::CamData();
	cdata.timeDelta = p_delta;
	m_sunLightCamera.Update(cdata);
}

bool CRasterRender::DoLightDepthPrepass(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][cb_ShadowMap];

	if (!BeginCommandBuffer(cmdBfr, "Shadow Map Generation"))
		return false;

	BeginRenderpass(m_lightDepthFrameBuffer, m_lightDepthRenderpass, cmdBfr);

	//InsertMarker(m_vkCmdBfr[p_swapchainIndex], "Ligth Depth Prepass");

	uint32_t fbWidth = m_lightDepthPipeline.renderpassData.framebufferWidth;
	uint32_t fbHeight = m_lightDepthPipeline.renderpassData.framebufferHeight;
	SetViewport(cmdBfr, 0.0f, 1.0f, (float)fbWidth, (float)fbHeight);
	SetScissors(cmdBfr, 0, 0, fbWidth, fbHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightDepthPipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightDepthPipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightDepthPipeline.pipeLayout, BindingSet::bs_Scene, 1, &m_scene.descSets, 0, nullptr);

	// Bind Index and Vertices buffers
	VkDeviceSize offsets[1] = { 0 };
	for (unsigned int i = MeshType::mt_Scene; i < m_scene.meshes.size(); i++)
	{
		RenderMesh mesh = m_scene.meshes[i];

		vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh.vertexBuffer.descInfo.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBfr, mesh.indexBuffer.descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

		for (uint32_t j = 0; j < mesh.submeshes.size(); j++)
		{
			SubMesh submesh = mesh.submeshes[j];

			PushConst pc{ mesh.mesh_id, submesh.materialId };

			VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(cmdBfr, m_deferredPipeline.pipeLayout, vertex_frag, 0, sizeof(PushConst), (void*)&pc);

			//uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(cmdBfr, submesh.indexCount, 1, submesh.firstIndex, 0, 1);
		}
	}

	EndRenderPass(cmdBfr);
	EndCommandBuffer(cmdBfr);
	cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::DoSkybox(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][cb_Forward];

	if(!BeginCommandBuffer(cmdBfr, "Skybox"))
		return false;

	BeginRenderpass(m_vkFrameBuffer[p_swapchainIndex], m_forwardRenderpass, cmdBfr);

	//InsertMarker(m_vkCmdBfr[p_swapchainIndex], "Skybox");

	SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_renderWidth, -(float)m_renderHeight);
	SetScissors(cmdBfr, 0, 0, m_renderWidth, m_renderHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipeline.pipeLayout, BindingSet::bs_Scene, 1, &m_scene.descSets, 0, nullptr);

	VkDeviceSize offsets[1] = { 0 };
	RenderMesh mesh = m_scene.meshes[MeshType::mt_Skybox];
	vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh.vertexBuffer.descInfo.buffer, offsets);
	vkCmdBindIndexBuffer(cmdBfr, mesh.indexBuffer.descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

	uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
	vkCmdDrawIndexed(cmdBfr, count, 1, 0, 0, 1);

	//EndRenderPass(cmdBfr);
	//EndCommandBuffer(cmdBfr);
	//cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::DoGBuffer_Forward(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][cb_Forward];

	//if(!BeginCommandBuffer(cmdBfr, "GBuffer_Forward"))
	//	return false;

	//BeginRenderpass(m_vkFrameBuffer[p_swapchainIndex], m_forwardRenderpass, cmdBfr);

	InsertMarker(cmdBfr, "GBuffer_Forward");

	SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_renderWidth, -(float)m_renderHeight);
	SetScissors(cmdBfr, 0, 0, m_renderWidth, m_renderHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_forwardPipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_forwardPipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_forwardPipeline.pipeLayout, BindingSet::bs_Scene, 1, &m_scene.descSets, 0, nullptr);

	// Bind Index and Vertices buffers
	VkDeviceSize offsets[1] = { 0 };
	for (unsigned int i = MeshType::mt_Scene; i < m_scene.meshes.size(); i++)
	{
		RenderMesh mesh = m_scene.meshes[i];

		vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh.vertexBuffer.descInfo.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBfr, mesh.indexBuffer.descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

		for (uint32_t j = 0; j < mesh.submeshes.size(); j++)
		{
			SubMesh submesh = mesh.submeshes[j];

			PushConst pc{ mesh.mesh_id, submesh.materialId };

			VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(cmdBfr, m_forwardPipeline.pipeLayout, vertex_frag, 0, sizeof(PushConst), (void*)&pc);

			//uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(cmdBfr, submesh.indexCount, 1, submesh.firstIndex, 0, 1);
		}
	}

	EndRenderPass(cmdBfr);
	EndCommandBuffer(cmdBfr);
	cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::DoGBuffer_Deferred(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][CommandBufferId::cb_Deferred_GBuf];
	
	if(!BeginCommandBuffer(cmdBfr, "Deferred GBuffer"))
		return false;

	BeginRenderpass(m_gFrameBuffer, m_deferredRenderpass, cmdBfr);

	SetViewport(cmdBfr, 0.0f, 1.0f, (float)m_renderWidth, -(float)m_renderHeight);
	SetScissors(cmdBfr, 0, 0, m_renderWidth, m_renderHeight);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredPipeline.pipeline);

	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredPipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredPipeline.pipeLayout, BindingSet::bs_Scene, 1, &m_scene.descSets, 0, nullptr);

	// Bind Index and Vertices buffers
	VkDeviceSize offsets[1] = { 0 };
	for (unsigned int i = MeshType::mt_Scene; i < m_scene.meshes.size(); i++)
	{
		RenderMesh mesh = m_scene.meshes[i];

		vkCmdBindVertexBuffers(cmdBfr, 0, 1, &mesh.vertexBuffer.descInfo.buffer, offsets);
		vkCmdBindIndexBuffer(cmdBfr, mesh.indexBuffer.descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

		for (uint32_t j = 0; j < mesh.submeshes.size(); j++)
		{
			SubMesh submesh = mesh.submeshes[j];

			PushConst pc{ mesh.mesh_id, submesh.materialId };

			VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			vkCmdPushConstants(cmdBfr, m_deferredPipeline.pipeLayout, vertex_frag, 0, sizeof(PushConst), (void*)&pc);

			//uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(cmdBfr, submesh.indexCount, 1, submesh.firstIndex, 0, 1);
		}
	}

	EndRenderPass(cmdBfr);
	EndCommandBuffer(cmdBfr);
	cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::DoSSAO(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][CommandBufferId::cb_SSAO];

	uint32_t dispatchDim_x = m_renderWidth / 8;
	uint32_t dispatchDim_y = m_renderHeight / 8;

	if(!BeginCommandBuffer(cmdBfr, "Compute SSAO"))
		return false;

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_ssaoComputePipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_ssaoComputePipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	InsertMarker(cmdBfr, "SSAO Blur Compute");
	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_ssaoBlurComputePipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_ssaoBlurComputePipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	EndCommandBuffer(cmdBfr);
	cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::DoDeferredLighting(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][CommandBufferId::cb_Deferred_Lighting];

	uint32_t dispatchDim_x = m_renderWidth / 8;
	uint32_t dispatchDim_y = m_renderHeight / 8;

	if(!BeginCommandBuffer(cmdBfr, "Compute Deferred Lighting"))
		return false;

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_deferredLightingPipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_deferredLightingPipeline.pipeLayout, BindingSet::bs_Primary, 1, &m_primaryDescSet[p_swapchainIndex], 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	EndCommandBuffer(cmdBfr);
	cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::DoReadBackObjPickerBuffer(uint32_t p_swapchainIndex)
{
	VkCommandBuffer cmdBfr = m_vkCmdBfr[p_swapchainIndex][CommandBufferId::cb_PickerCopy2CPU];

	if(!BeginCommandBuffer(cmdBfr, "ReadBack Object Picker"))
		return false;

	// Barrier to ensure that shader writes are finished before buffer is read back from GPU
	IssueBufferBarrier(VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, m_fixedBufferList[fb_ObjectPickerWrite].descInfo.buffer, cmdBfr);

	// Read back to host visible buffer
	VkBufferCopy copyRegion = {};
	copyRegion.size = m_fixedBufferList[fb_ObjectPickerWrite].descInfo.range;

	vkCmdCopyBuffer(cmdBfr, m_fixedBufferList[fb_ObjectPickerWrite].descInfo.buffer, m_fixedBufferList[fb_ObjectPickerRead].descInfo.buffer, 1, &copyRegion);

	// Barrier to ensure that buffer copy is finished before host reading from it
	IssueBufferBarrier(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_HOST_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, m_fixedBufferList[fb_ObjectPickerRead].descInfo.buffer, cmdBfr);

	EndCommandBuffer(cmdBfr);
	cmdBfrsInUse.push_back(cmdBfr);

	return true;
}

bool CRasterRender::CreateSyncPremitives()
{
	if (!CreateSemaphor(m_vkswapchainAcquireSemaphore))
		return false;

	if (!CreateSemaphor(m_vksubmitCompleteSemaphore))
		return false;

	if (!CreateFence(VK_FENCE_CREATE_SIGNALED_BIT, m_vkFenceCmdBfrFree[0]))
		return false;

	if (!CreateFence(VK_FENCE_CREATE_SIGNALED_BIT, m_vkFenceCmdBfrFree[1]))
		return false;

	return true;
}

bool CRasterRender::CreateFixedResources()
{
	VkCommandBuffer layoutTransCmdBfr;
	if (!CreateCommandBuffers(m_vkCmdPool, &layoutTransCmdBfr, 1))
		return false;
	if (!BeginCommandBuffer(layoutTransCmdBfr, "Fixed Resource Prep"))
		return false;

	size_t primaryUniformBufferSize =
		(sizeof(float) * 1)	// delta time elapsed
		+ (sizeof(float) * 3)	// camera lookfrom
		+ (sizeof(float) * 16)	// camera view projection
		+ (sizeof(float) * 16)	// camera projection
		+ (sizeof(float) * 16)	// camera view
		+ (sizeof(float) * 16)	// inverse camera view
		+ (sizeof(float) * 16)	// skybox model view
		+ (sizeof(float) * 2)	// render resolution
		+ (sizeof(float) * 2)	// mouse position (x,y)
		+ (sizeof(float) * 2)	// SSAO Noise Scale
		+ (sizeof(float) * 1)	// SSAO Kernel Size
		+ (sizeof(float) * 1)	// SSAO Radius
		+ (sizeof(float) * 16)	// sun light View Projection
		+ (sizeof(float) * 3)	// sun light direction in world space
		+ (sizeof(float) * 1)	// unassigned_0
		+ (sizeof(float) * 3)	// sun light direction in view space
		+ (sizeof(float) * 1);	// unassigned_1

	size_t objPickerBufferSize = sizeof(uint32_t) * 1; // selected mesh ID

	struct ResourceBufferData { uint32_t id; VkBufferUsageFlags usage; VkMemoryPropertyFlags memProp; size_t size; void* data;};
	std::vector<ResourceBufferData> fixedResourceList
	{
			{fb_PrimaryUniform_0,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, primaryUniformBufferSize,	nullptr}
		,	{fb_PrimaryUniform_1,	VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, primaryUniformBufferSize,	nullptr}
		,	{fb_ObjectPickerRead,	VK_BUFFER_USAGE_TRANSFER_DST_BIT,										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,										objPickerBufferSize,		nullptr}
		,	{fb_ObjectPickerWrite,	VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,										objPickerBufferSize,		nullptr}
	};

	for (const auto& res : fixedResourceList)
	{
		if (!CreateResourceBuffer(res.data, res.size, m_fixedBufferList[res.id], res.usage, res.memProp))
			return false;
	}

	struct RenderTargetData { uint32_t id; VkFormat format;	uint32_t width; uint32_t height; VkImageLayout layout; VkImageUsageFlags usage; };
	std::vector<RenderTargetData> rtDataList
	{
			{rt_PrimaryDepth,		VK_FORMAT_D32_SFLOAT_S8_UINT,	m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT}
		,	{rt_Position,			VK_FORMAT_R32G32B32A32_SFLOAT,	m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}
		,	{rt_Normal,				VK_FORMAT_R32G32B32A32_SFLOAT,	m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}
		,	{rt_Albedo,				VK_FORMAT_R32G32B32A32_SFLOAT,	m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}
		,	{rt_SSAO,				VK_FORMAT_R32_SFLOAT,			m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT}
		,	{rt_SSAOBlur,			VK_FORMAT_R32_SFLOAT,			m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT}
		,	{rt_LightDepth,			VK_FORMAT_D32_SFLOAT_S8_UINT,	4096,		   4096,		   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT}
		,	{rt_DeferredLighting,	VK_FORMAT_R16G16B16A16_SFLOAT,	m_renderWidth, m_renderHeight, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT}
	};

	for (const auto& rt : rtDataList)
	{
		if (!CreateRenderTarget(rt.format, rt.width, rt.height, rt.layout, rt.usage, m_renderTargets[rt.id]))
			return false;
	}

	struct SamplerData { uint32_t id; VkFilter filter; VkImageLayout layout; };
	std::vector<SamplerData> samplerDataList
	{
		{s_Linear, VK_FILTER_LINEAR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
	};

	for (const auto& samp : samplerDataList)
	{
		m_samplers[samp.id].filter					= samp.filter;
		m_samplers[samp.id].maxAnisotropy			= 1.0f;
		m_samplers[samp.id].descInfo.imageLayout	= samp.layout;
		m_samplers[samp.id].descInfo.imageView		= VK_NULL_HANDLE;
		if (!CreateSampler(m_samplers[samp.id]))
			return false;
	}

	vkEndCommandBuffer(layoutTransCmdBfr);

	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &layoutTransCmdBfr;
	submitInfo.pSignalSemaphores	= nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pWaitSemaphores		= nullptr;
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.pWaitDstStageMask	= &waitstage;
	if (!SubmitCommandbuffer(m_vkQueue[0], &submitInfo, 1))
		return false;

	if (!WaitToFinish(m_vkQueue[0]))
		return false;

	return true;
}

bool CRasterRender::CreateRenderTarget(VkFormat p_format, uint32_t p_width, uint32_t p_height, VkImageLayout p_iniLayout, VkImageUsageFlags p_usage, Image& p_renderTarget)
{
	{
		VkFormatFeatureFlags feature;
		if (p_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			feature |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		if (p_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			feature |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
		if(p_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT || p_usage & VK_IMAGE_USAGE_SAMPLED_BIT)
			feature |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		if (p_usage & VK_IMAGE_USAGE_STORAGE_BIT)
			feature |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

		if (!IsFormatSupported(p_format, feature))
			return false;

		p_renderTarget.format				= p_format;
		p_renderTarget.descInfo.imageLayout = p_iniLayout;
		p_renderTarget.height				= p_height;
		p_renderTarget.width				= p_width;

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.extent			= VkExtent3D{ p_width, p_height, 1 };
		imageCreateInfo.format			= p_renderTarget.format;
		imageCreateInfo.imageType		= VK_IMAGE_TYPE_2D;
		imageCreateInfo.tiling			= VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.mipLevels		= 1;
		imageCreateInfo.arrayLayers		= 1;
		imageCreateInfo.initialLayout	= p_renderTarget.descInfo.imageLayout;
		imageCreateInfo.usage			= p_usage;
		imageCreateInfo.sharingMode		= VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.samples			= VK_SAMPLE_COUNT_1_BIT;

		if (!CreateImage(imageCreateInfo, p_renderTarget.image))
			return false;
		if (!AllocateImageMemory(p_renderTarget.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_renderTarget.devMem))
			return false;
		if (!BindImageMemory(p_renderTarget.image, p_renderTarget.devMem))
			return false;
		if (!CreateImagView(p_usage, p_renderTarget.image, p_renderTarget.format, VK_IMAGE_VIEW_TYPE_2D, p_renderTarget.descInfo.imageView))
			return false;
	}

	return true;
}

bool CRasterRender::CreateLightDepthRenderpass()
{
	m_lightDepthRenderpass.AttachDepth(m_renderTargets[rt_LightDepth].format,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	m_lightDepthRenderpass.framebufferWidth = m_renderTargets[rt_LightDepth].width;
	m_lightDepthRenderpass.framebufferHeight = m_renderTargets[rt_LightDepth].height;
	if (!CreateRenderpass(m_lightDepthRenderpass))
		return false;

	std::vector<VkImageView> attachments(1, VkImageView{});
	attachments[0] = m_renderTargets[rt_LightDepth].descInfo.imageView;
	if (!CreateFramebuffer(m_lightDepthRenderpass.renderpass, m_lightDepthFrameBuffer, attachments.data(), (uint32_t)attachments.size(), m_renderTargets[rt_LightDepth].width, m_renderTargets[rt_LightDepth].height))
		return false;

	return true;
}

bool CRasterRender::CreateReadOnlyResources()
{
	std::vector<Buffer> staginBufferList;

	VkCommandBuffer transferCmdBfr;
	if (!CreateCommandBuffers(m_vkCmdPool, &transferCmdBfr, 1))
		return false;
	if (!BeginCommandBuffer(transferCmdBfr, "Read Only Resources Loading"))
		return false;

	m_readonlyBufferList.resize(BufferReadOnlyId::br_max);
	m_readonlyTextureList.resize(TextureReadOnlyId::tr_max);

	{
		Buffer stg1, stg2;
		if (!CreateSSAOResources(stg1, stg2, transferCmdBfr))
			return false;

		staginBufferList.push_back(stg1);
		staginBufferList.push_back(stg2);
	}

	if (!EndCommandBuffer(transferCmdBfr))
		return false;

	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &transferCmdBfr;
	submitInfo.pSignalSemaphores	= nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pWaitSemaphores		= nullptr;
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.pWaitDstStageMask	= &waitstage;
	if (!SubmitCommandbuffer(m_vkQueue[0], &submitInfo, 1))
		return false;

	if (!WaitToFinish(m_vkQueue[0]))
		return false;

	for (auto& stg : staginBufferList)
		DestroyBuffer(stg.descInfo.buffer);

	return true;
}

bool CRasterRender::CreateForwardRenderpass()
{
	m_forwardRenderpass.AttachColor(VK_FORMAT_B8G8R8A8_UNORM,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_forwardRenderpass.AttachDepth(m_renderTargets[rt_PrimaryDepth].format,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	if (!CreateRenderpass(m_forwardRenderpass))
		return false;

	m_forwardRenderpass.framebufferWidth = m_renderWidth;
	m_forwardRenderpass.framebufferHeight = m_renderHeight;
	std::vector<VkImageView> attachments(2, VkImageView{});
	attachments[0] = m_swapchainImageViewList[0];
	attachments[1] = m_renderTargets[rt_PrimaryDepth].descInfo.imageView;
	if (!CreateFramebuffer(m_forwardRenderpass.renderpass, m_vkFrameBuffer[0], attachments.data(), (uint32_t)attachments.size(), m_renderWidth, m_renderHeight))
		return false;

	attachments[0] = m_swapchainImageViewList[1];
	if (!CreateFramebuffer(m_forwardRenderpass.renderpass, m_vkFrameBuffer[1], attachments.data(), (uint32_t)attachments.size(), m_renderWidth, m_renderHeight))
		return false;

	return true;
}

bool CRasterRender::CreateDeferredRenderpass()
{
	m_deferredRenderpass.AttachColor(m_renderTargets[rt_Position].format,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_deferredRenderpass.AttachColor(m_renderTargets[rt_Normal].format,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_deferredRenderpass.AttachColor(m_renderTargets[rt_Albedo].format,
		VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	m_deferredRenderpass.AttachDepth(m_renderTargets[rt_PrimaryDepth].format,
		VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	if (!CreateRenderpass(m_deferredRenderpass))
		return false;

	std::vector<VkImageView> attachments;
	attachments.push_back(m_renderTargets[rt_Position].descInfo.imageView);
	attachments.push_back(m_renderTargets[rt_Normal].descInfo.imageView);
	attachments.push_back(m_renderTargets[rt_Albedo].descInfo.imageView);
	attachments.push_back(m_renderTargets[rt_PrimaryDepth].descInfo.imageView);

	m_deferredRenderpass.framebufferWidth = m_renderWidth;
	m_deferredRenderpass.framebufferHeight = m_renderHeight;
	if (!CreateFramebuffer(m_deferredRenderpass.renderpass, m_gFrameBuffer, attachments.data(), (uint32_t)attachments.size(), m_renderWidth, m_renderHeight))
		return false;

	return true;
}

bool CRasterRender::CreatePipelines()
{
	VkPushConstantRange pushrange{};
	pushrange.offset						= 0;
	pushrange.size							= sizeof(PushConst);	// pushing mesh and submesh ID
	pushrange.stageFlags					= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayout primaryAndSceneLayout;
	std::vector<VkDescriptorSetLayout> descLayouts;
	descLayouts.push_back(m_primaryDescLayout[0]);	// Set 0 - BindingSet::Primary
	descLayouts.push_back(m_scene.descLayout);		// Set 1 - BindingSet::Mesh
	if (!CreatePipelineLayout(&pushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), primaryAndSceneLayout))
		return false;

	VkPipelineLayout primaryLayout;
	descLayouts.clear();
	descLayouts.push_back(m_primaryDescLayout[0]);	// Set 0 - BindingSet::Primary
	if (!CreatePipelineLayout(&pushrange, 1, descLayouts.data(), (uint32_t)descLayouts.size(), primaryLayout))
		return false;

	VkVertexInputBindingDescription vertexInputBinding = {};
	vertexInputBinding.binding				= 0;
	vertexInputBinding.stride				= sizeof(Vertex);
	vertexInputBinding.inputRate			= VK_VERTEX_INPUT_RATE_VERTEX;

	//	layout (location = 0) in vec3 pos;
	//	layout (location = 1) in vec3 normal;
	//	layout (location = 2) in vec2 uv;
	//	layout (location = 3) in vec3 color;
	std::vector<VkVertexInputAttributeDescription> vertexAttributs;
	// Attribute location 0: pos
	VkVertexInputAttributeDescription attribDesc{};
	attribDesc.binding						= 0;
	attribDesc.location						= 0;
	attribDesc.format						= VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset						= offsetof(Vertex, pos);
	vertexAttributs.push_back(attribDesc);

	// Attribute location 1: normal
	attribDesc.binding						= 0;
	attribDesc.location						= 1;
	attribDesc.format						= VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset						= offsetof(Vertex, normal);
	vertexAttributs.push_back(attribDesc);

	// Attribute location 2: uv
	attribDesc.binding	= 0;
	attribDesc.location						= 2;
	attribDesc.format						= VK_FORMAT_R32G32_SFLOAT;
	attribDesc.offset						= offsetof(Vertex, uv);
	vertexAttributs.push_back(attribDesc);

	// Attribute location 3: tangent
	attribDesc.binding	= 0;
	attribDesc.location						= 3;
	attribDesc.format						= VK_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc.offset						= offsetof(Vertex, tangent);
	vertexAttributs.push_back(attribDesc);

	m_forwardPipeline.pipeLayout			= primaryAndSceneLayout;
	m_forwardPipeline.shaderpath_vertex		= g_EnginePath + "/shaders/spirv/Forward.vert.spv";
	m_forwardPipeline.shaderpath_fragment	= g_EnginePath + "/shaders/spirv/Forward.frag.spv";
	m_forwardPipeline.vertexInBinding		= vertexInputBinding;
	m_forwardPipeline.vertexAttributeDesc	= vertexAttributs;
	m_forwardPipeline.cullMode				= VK_CULL_MODE_BACK_BIT;
	m_forwardPipeline.enableDepthTest		= true;
	m_forwardPipeline.enableDepthWrite		= true;
	m_forwardPipeline.renderpassData		= m_forwardRenderpass;
	if (!CreateGraphicsPipeline(m_forwardPipeline))
		return false;

	m_skyboxPipeline.pipeLayout				= primaryAndSceneLayout;
	m_skyboxPipeline.shaderpath_vertex		= g_EnginePath + "/shaders/spirv/Skybox.vert.spv";
	m_skyboxPipeline.shaderpath_fragment	= g_EnginePath + "/shaders/spirv/Skybox.frag.spv";
	m_skyboxPipeline.vertexInBinding		= vertexInputBinding;
	m_skyboxPipeline.vertexAttributeDesc	= vertexAttributs;
	m_skyboxPipeline.cullMode				= VK_CULL_MODE_NONE;
	m_skyboxPipeline.enableDepthTest		= false;
	m_skyboxPipeline.enableDepthWrite		= false;
	m_skyboxPipeline.pipeLayout				= m_forwardPipeline.pipeLayout;
	m_skyboxPipeline.renderpassData			= m_forwardRenderpass;
	if (!CreateGraphicsPipeline(m_skyboxPipeline))
		return false;

	m_deferredPipeline.pipeLayout			= primaryAndSceneLayout;
	m_deferredPipeline.shaderpath_vertex	= g_EnginePath + "/shaders/spirv/Deferred_Gbuffer.vert.spv";
	m_deferredPipeline.shaderpath_fragment	= g_EnginePath + "/shaders/spirv/Deferred_Gbuffer.frag.spv";
	m_deferredPipeline.vertexInBinding		= vertexInputBinding;
	m_deferredPipeline.vertexAttributeDesc	= vertexAttributs;
	m_deferredPipeline.cullMode				= VK_CULL_MODE_BACK_BIT;
	m_deferredPipeline.enableDepthTest		= true;
	m_deferredPipeline.enableDepthWrite		= false;
	m_deferredPipeline.depthCmpOp			= VK_COMPARE_OP_EQUAL;
	m_deferredPipeline.pipeLayout			= m_forwardPipeline.pipeLayout;
	m_deferredPipeline.renderpassData		= m_deferredRenderpass;
	if (!CreateGraphicsPipeline(m_deferredPipeline))
		return false;

	m_lightDepthPipeline.pipeLayout			= primaryAndSceneLayout;
	m_lightDepthPipeline.shaderpath_vertex	= g_EnginePath + "/shaders/spirv/LightDepthPrepass.vert.spv";
	m_lightDepthPipeline.vertexInBinding	= vertexInputBinding;
	m_lightDepthPipeline.vertexAttributeDesc = vertexAttributs;
	m_lightDepthPipeline.cullMode			= VK_CULL_MODE_BACK_BIT;
	m_lightDepthPipeline.enableDepthTest	= true;
	m_lightDepthPipeline.enableDepthWrite	= true;
	m_lightDepthPipeline.renderpassData		= m_lightDepthRenderpass;
	if (!CreateGraphicsPipeline(m_lightDepthPipeline))
		return false;

	m_ssaoComputePipeline.pipeLayout			= primaryLayout;
	m_ssaoComputePipeline.shaderpath_compute	= g_EnginePath + "/shaders/spirv/SSAO.comp.spv";
	if (!CreateComputePipeline(m_ssaoComputePipeline))
		return false;

	m_ssaoBlurComputePipeline.pipeLayout			= primaryLayout;
	m_ssaoBlurComputePipeline.shaderpath_compute	= g_EnginePath + "/shaders/spirv/SSAOBlur.comp.spv";
	if (!CreateComputePipeline(m_ssaoBlurComputePipeline))
		return false;

	m_deferredLightingPipeline.pipeLayout			= primaryLayout;
	m_deferredLightingPipeline.shaderpath_compute	= g_EnginePath + "/shaders/spirv/Deferred_Lighting.comp.spv";
	if (!CreateComputePipeline(m_deferredLightingPipeline))
		return false;

	return true;
}

bool CRasterRender::CreatePrimaryDescriptors()
{
	// read only tex desc info
	std::vector<VkDescriptorImageInfo> readTexDesInfoList;
	for (const auto& readTex : m_readonlyTextureList)
		readTexDesInfoList.push_back(readTex.descInfo);

	DescDataList descriptorData
	{
		  {BindingDest::bd_Gloabl_Uniform,			1,			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_ALL,			&m_fixedBufferList[fb_PrimaryUniform_0].descInfo,	VK_NULL_HANDLE }
		, {BindingDest::bd_Linear_Sampler,			1,			VK_DESCRIPTOR_TYPE_SAMPLER,			VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,										&m_samplers[s_Linear].descInfo}
		, {BindingDest::bd_ObjPicker_Storage,		1,			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT,	&m_fixedBufferList[fb_ObjectPickerWrite].descInfo,	VK_NULL_HANDLE}
		, {BindingDest::bd_Depth_Image,				1,			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_PrimaryDepth].descInfo}
		, {BindingDest::bd_PosGBuf_Image,			1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_Position].descInfo}
		, {BindingDest::bd_NormGBuf_Image,			1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_Normal].descInfo}
		, {BindingDest::bd_AlbedoGBuf_Image,		1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_Albedo].descInfo}
		, {BindingDest::bd_SSAO_Image,				1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_SSAO].descInfo}
		, {BindingDest::bd_SSAOBlur_Image,			1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_SSAOBlur].descInfo}
		, {BindingDest::bd_SSAOKernel_Storage,		1,			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_COMPUTE_BIT,	&m_readonlyBufferList[br_SSAOKernel].descInfo,		VK_NULL_HANDLE}
		, {BindingDest::bd_LightDepth_Image,		1,			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,										&m_renderTargets[rt_LightDepth].descInfo}
		, {BindingDest::bd_DeferredLighting_Image,	1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_DeferredLighting].descInfo}
		, {BindingDest::bd_PrimaryRead_TexArray,	tr_max,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										readTexDesInfoList.data() }
	};

	std::vector<VkDescriptorPoolSize> dPSize;
	std::vector<VkDescriptorSetLayoutBinding> dsLayoutBinding;
	for (const auto& desc : descriptorData)
	{
		dPSize.push_back(VkDescriptorPoolSize{ desc.type, desc.count * FRAME_BUFFER_COUNT });
		dsLayoutBinding.push_back(VkDescriptorSetLayoutBinding{ desc.bindingDest, desc.type, desc.count, desc.shaderStage });
	}

	if (!CreateDescriptorPool(dPSize.data(), (uint32_t)dPSize.size(), m_primaryDescPool))
		return false;

	std::vector<VkDescriptorBindingFlagsEXT> descBindFlags(bd_Primary_max, 0);
	descBindFlags[bd_PrimaryRead_TexArray] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layoutBindFlags{};
	layoutBindFlags.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	layoutBindFlags.bindingCount	= (uint32_t)descBindFlags.size();
	layoutBindFlags.pBindingFlags	= descBindFlags.data();

	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		if (!CreateDescriptorSetLayout(dsLayoutBinding.data(), (uint32_t)dsLayoutBinding.size(), m_primaryDescLayout[i], &layoutBindFlags))
			return false;
	}

	uint32_t varDescCount[] = { (uint32_t)(m_readonlyTextureList.size()) };
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT varDescCountAllocInfo = {};
	varDescCountAllocInfo.sType					= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	varDescCountAllocInfo.descriptorSetCount	= 1;
	varDescCountAllocInfo.pDescriptorCounts		= varDescCount;

	if (!CreateDescriptors(descriptorData, m_primaryDescPool, &m_primaryDescLayout[0], 1, &m_primaryDescSet[0], &varDescCountAllocInfo))
		return false;

	DescDataList descriptorData2
	{
		  {BindingDest::bd_Gloabl_Uniform,			1,			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,	VK_SHADER_STAGE_ALL,			&m_fixedBufferList[fb_PrimaryUniform_1].descInfo,	VK_NULL_HANDLE }
		, {BindingDest::bd_Linear_Sampler,			1,			VK_DESCRIPTOR_TYPE_SAMPLER,			VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,										&m_samplers[s_Linear].descInfo}
		, {BindingDest::bd_ObjPicker_Storage,		1,			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_FRAGMENT_BIT,	&m_fixedBufferList[fb_ObjectPickerWrite].descInfo,	VK_NULL_HANDLE}
		, {BindingDest::bd_Depth_Image,				1,			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_PrimaryDepth].descInfo}
		, {BindingDest::bd_PosGBuf_Image,			1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_Position].descInfo}
		, {BindingDest::bd_NormGBuf_Image,			1,			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_Normal].descInfo}
		, {BindingDest::bd_AlbedoGBuf_Image,		1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_Albedo].descInfo}
		, {BindingDest::bd_SSAO_Image,				1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_SSAO].descInfo}
		, {BindingDest::bd_SSAOBlur_Image,			1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_SSAOBlur].descInfo}
		, {BindingDest::bd_SSAOKernel_Storage,		1,			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,	VK_SHADER_STAGE_COMPUTE_BIT,	&m_readonlyBufferList[br_SSAOKernel].descInfo,		VK_NULL_HANDLE}
		, {BindingDest::bd_LightDepth_Image,		1,			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_ALL,			VK_NULL_HANDLE,										&m_renderTargets[rt_LightDepth].descInfo}
		, {BindingDest::bd_DeferredLighting_Image,	1,			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										&m_renderTargets[rt_DeferredLighting].descInfo}
		, {BindingDest::bd_PrimaryRead_TexArray,	tr_max,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	VK_SHADER_STAGE_COMPUTE_BIT,	VK_NULL_HANDLE,										readTexDesInfoList.data() }
	};

	if (!CreateDescriptors(descriptorData, m_primaryDescPool, &m_primaryDescLayout[1], 1, &m_primaryDescSet[1], &varDescCountAllocInfo))
		return false;

	return true;
}

bool CRasterRender::CreateSceneDescriptors()
{
	std::vector<VkDescriptorImageInfo> imageInfoList;
	for (const auto& imgInfo : m_scene.textures)
		imageInfoList.push_back(imgInfo.descInfo);
	
	uint32_t texturesCount = (uint32_t)m_scene.textures.size();
	VkPipelineStageFlags vertex_frag = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	DescDataList descriptorData
	{ 
		  {BindingDest::bd_Scene_MeshInfo_Uniform,			1,	VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,			vertex_frag,					&m_scene.meshInfo_uniform.descInfo,			VK_NULL_HANDLE }
		, {BindingDest::bd_CubeMap_Texture,					1,	VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,	VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,								& m_scene.skybox_cubemap.descInfo}
		, {BindingDest::bd_Material_Storage,				1,	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,			VK_SHADER_STAGE_FRAGMENT_BIT,	&m_scene.material_storage.descInfo,			VK_NULL_HANDLE }
		, {BindingDest::bd_SceneRead_TexArray,	texturesCount,	VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,			VK_SHADER_STAGE_FRAGMENT_BIT,	VK_NULL_HANDLE,								imageInfoList.data() }
	};

	std::vector<VkDescriptorPoolSize> dPSize;
	std::vector<VkDescriptorSetLayoutBinding> dsLayoutBinding;
	for (const auto& desc : descriptorData)
	{
		dPSize.push_back(VkDescriptorPoolSize{ desc.type, desc.count });
		dsLayoutBinding.push_back(VkDescriptorSetLayoutBinding{ desc.bindingDest, desc.type, desc.count, desc.shaderStage });
	}

	if (!CreateDescriptorPool(dPSize.data(), (uint32_t)dPSize.size(), m_scene.descPool))
		return false;

	std::vector<VkDescriptorBindingFlagsEXT> descBindFlags(bd_Scene_max, 0);
	descBindFlags[bd_SceneRead_TexArray] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT layoutBindFlags{};
	layoutBindFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	layoutBindFlags.bindingCount = (uint32_t)descBindFlags.size();
	layoutBindFlags.pBindingFlags = descBindFlags.data();

	if (!CreateDescriptorSetLayout(dsLayoutBinding.data(), (uint32_t)dsLayoutBinding.size(), m_scene.descLayout, &layoutBindFlags))
		return false;

	uint32_t varDescCount[] = { (uint32_t)(m_scene.textures.size()) };
	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT varDescCountAllocInfo = {};
	varDescCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	varDescCountAllocInfo.descriptorSetCount = 1;
	varDescCountAllocInfo.pDescriptorCounts = varDescCount;

	if (!CreateDescriptors(descriptorData, m_scene.descPool, &m_scene.descLayout, 1, &m_scene.descSets, &varDescCountAllocInfo))
			return false;

	return true;
}

void CRasterRender::SetLayoutForDescriptorCreaton()
{
	m_renderTargets[rt_DeferredLighting].descInfo.imageLayout	= VK_IMAGE_LAYOUT_GENERAL;
	m_renderTargets[rt_LightDepth].descInfo.imageLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_renderTargets[rt_SSAOBlur].descInfo.imageLayout			= VK_IMAGE_LAYOUT_GENERAL;
	m_renderTargets[rt_SSAO].descInfo.imageLayout				= VK_IMAGE_LAYOUT_GENERAL;
	m_renderTargets[rt_Position].descInfo.imageLayout			= VK_IMAGE_LAYOUT_GENERAL;
	m_renderTargets[rt_Albedo].descInfo.imageLayout				= VK_IMAGE_LAYOUT_GENERAL;
	m_renderTargets[rt_Normal].descInfo.imageLayout				= VK_IMAGE_LAYOUT_GENERAL;
	m_renderTargets[rt_PrimaryDepth].descInfo.imageLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void CRasterRender::UnsetLayoutForDescriptorCreaton()
{
	m_renderTargets[rt_DeferredLighting].descInfo.imageLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	m_renderTargets[rt_SSAOBlur].descInfo.imageLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	m_renderTargets[rt_SSAO].descInfo.imageLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	m_renderTargets[rt_Position].descInfo.imageLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
	m_renderTargets[rt_Albedo].descInfo.imageLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	m_renderTargets[rt_Normal].descInfo.imageLayout				= VK_IMAGE_LAYOUT_UNDEFINED;
	m_renderTargets[rt_PrimaryDepth].descInfo.imageLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
}

bool CRasterRender::CreateDescriptors(const DescDataList& p_descdataList, VkDescriptorPool& p_descPool,
	VkDescriptorSetLayout* p_descLayout, uint32_t p_layoutCount, VkDescriptorSet* p_desc, void* p_next)
{
	if (!AllocateDescriptorSets(p_descPool, p_descLayout, p_layoutCount, p_desc, p_next))
		return false;
	
	std::vector<VkWriteDescriptorSet> writeDescList;
	for (const auto& desc : p_descdataList)
	{
		//for (int i = 0; i < desc.count; i++)
		{
			if (desc.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER 
				|| desc.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			{
				VkWriteDescriptorSet wdSet{};
				wdSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wdSet.pNext = nullptr;
				wdSet.dstSet = *p_desc;
				wdSet.descriptorCount = desc.count;
				wdSet.descriptorType = desc.type;
				wdSet.dstBinding = desc.bindingDest;
				wdSet.pBufferInfo = desc.bufDesInfo; // since we need 1 uniform descripotr per frame buffer; setting accordinigly

				writeDescList.push_back(wdSet);
			}

			if (desc.type == VK_DESCRIPTOR_TYPE_SAMPLER
				|| desc.type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
				|| desc.type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
				|| desc.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				VkWriteDescriptorSet wdSet{};
				wdSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wdSet.pNext = nullptr;
				wdSet.dstSet = *p_desc;
				wdSet.descriptorCount = desc.count;
				wdSet.descriptorType = desc.type;
				wdSet.dstBinding = desc.bindingDest;
				wdSet.pImageInfo = desc.imgDesinfo;

				writeDescList.push_back(wdSet);
			}
		}
	}
	
	vkUpdateDescriptorSets(m_vkDevice, (uint32_t)writeDescList.size(), writeDescList.data(), 0, nullptr);

	return true;
}

bool CRasterRender::CreateSSAOResources(Buffer& p_kernelstg, Buffer& p_noisestg, VkCommandBuffer p_cmdBfr)
{
	{
		std::vector<nm::float4> ssaoKernel;
		for (uint32_t i = 0; i < m_ssaokernelSize; i++)
		{
			// creating random values along a semi-sphere oriented along surface normal (Z axis) in tangent space
			nm::float4 sample = {
					randf() * 2.0f - 1.0f
				,	randf() * 2.0f - 1.0f
				,	randf()
				,	0.0f};
			
			sample = nm::normalize(sample);
			sample = randf() * sample;

			// we also want to make sure the random 3d positions are placed closer to the actual fragment
			float scale = (float)i / 64.0f;
			scale = 0.01f + (scale * scale) * (1.0f - 0.01f); // exponential curve forcing values close to frag
			sample = scale * sample;

			ssaoKernel.push_back(sample);
		}
	
		if (!CreateResourceBuffer(ssaoKernel.data(), sizeof(float) * 4 * ssaoKernel.size(), p_kernelstg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		if (!CreateLoadedBuffer(p_kernelstg, m_readonlyBufferList[br_SSAOKernel], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, p_cmdBfr))
			return false;
	}

	{
		std::vector<unsigned char> ssaoNoise;
		// introducing good random rotation can reduce number of samples required
		for (uint32_t i = 0; i < m_ssaoNoiseDimSquared; i++)
		{
			ssaoNoise.push_back((unsigned char)(randf() * 255.0f));
			ssaoNoise.push_back((unsigned char)(randf() * 255.0f));
			ssaoNoise.push_back((unsigned char)0);
			ssaoNoise.push_back((unsigned char)0);// making sure the roation happens along the z axis only
		}

		size_t texSize = m_ssaoNoiseDimSquared * 4 * sizeof(float);
		if (!CreateResourceBuffer(ssaoNoise.data(), texSize, p_noisestg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		m_readonlyTextureList[tr_SSAONoise].format = VK_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R16G16B16A16_SNORM;
		m_readonlyTextureList[tr_SSAONoise].width = (uint32_t)sqrt(m_ssaoNoiseDimSquared);
		m_readonlyTextureList[tr_SSAONoise].height = (uint32_t)sqrt(m_ssaoNoiseDimSquared);
		m_readonlyTextureList[tr_SSAONoise].viewType = VK_IMAGE_VIEW_TYPE_2D;

		VkImageCreateInfo imgCrtInfo	= CVulkanCore::ImageCreateInfo();
		imgCrtInfo.extent.width			= m_readonlyTextureList[tr_SSAONoise].width;
		imgCrtInfo.extent.height		= m_readonlyTextureList[tr_SSAONoise].height;
		imgCrtInfo.format				= m_readonlyTextureList[tr_SSAONoise].format;
		imgCrtInfo.usage				= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		if (!CreateTexture(p_noisestg, m_readonlyTextureList[tr_SSAONoise], imgCrtInfo, p_cmdBfr))
			return false;
	}
	
	return true;
}

bool CRasterRender::CreateResourceBuffer(void* p_data, size_t p_size, Buffer& p_dest, VkBufferUsageFlags p_bfrUsg, VkMemoryPropertyFlags p_propFlag)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage					= p_bfrUsg;
	bufferCreateInfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.pQueueFamilyIndices	= &m_QFIndex;
	bufferCreateInfo.queueFamilyIndexCount	= 1;
	bufferCreateInfo.size					= p_size;

	p_dest.descInfo.range	= p_size;
	p_dest.descInfo.offset	= 0;

	if (!CreateBuffer(bufferCreateInfo, p_dest.descInfo.buffer))
		return false;
	if (!AllocateBufferMemory(p_dest.descInfo.buffer, p_propFlag, p_dest.devMem))
		return false;
	
	if (p_data != nullptr)
	{
		if (!UploadDataToBuffer((uint8_t*)p_data, p_dest))
			return false;
	}

	if (!BindBufferMemory(p_dest.descInfo.buffer, p_dest.devMem))
		return false;
	
	return true;
}

bool CRasterRender::CreateLoadedBuffer(Buffer& p_staging, Buffer& p_buffer, VkBufferUsageFlags p_bfrUsg, VkCommandBuffer& p_cmdBfr)
{
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.usage					= p_bfrUsg;
	bufferCreateInfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.pQueueFamilyIndices	= &m_QFIndex;
	bufferCreateInfo.queueFamilyIndexCount	= 1;
	bufferCreateInfo.size = p_staging.descInfo.range;

	p_buffer.descInfo.range		= p_staging.descInfo.range;
	p_buffer.descInfo.offset	= 0;

	if (!CreateBuffer(bufferCreateInfo, p_buffer.descInfo.buffer))
		return false;
	if (!AllocateBufferMemory(p_buffer.descInfo.buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_buffer.devMem))
		return false;
	if (!BindBufferMemory(p_buffer.descInfo.buffer, p_buffer.devMem))
		return false;
	if (!UploadStagingToBuffer(p_staging, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_buffer, p_cmdBfr))
		return false;

	return true;
}

bool CRasterRender::CreateTexture(Buffer& p_staging, Image& p_Image, VkImageCreateInfo p_createInfo, VkCommandBuffer& p_cmdBfr)
{
	p_Image.descInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // final layout

	if (!CreateImage(p_createInfo, p_Image.image))
		return false;
	if (!AllocateImageMemory(p_Image.image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, p_Image.devMem))
		return false;
	if (!BindImageMemory(p_Image.image, p_Image.devMem))
		return false;

	IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image.layerCount, p_Image.image, p_cmdBfr);
	UploadStagingToImage(p_staging, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image, p_cmdBfr);
	IssueImageLayoutBarrier(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_Image.descInfo.imageLayout, p_Image.layerCount, p_Image.image, p_cmdBfr);
	
	if (!CreateImagView(p_createInfo.usage, p_Image.image, p_Image.format, p_Image.viewType, p_Image.descInfo.imageView))
		return false;

	return true;
}

bool CRasterRender::CreateMeshUniformBuffer()
{
	size_t uniBufize = m_scene.meshes.size() * (
		(sizeof(float) * 16)	// model matrix
	+	(sizeof(float) * 16)	// tranpose(inverse(model)) for transforming normal to world space
		);

	if (!CreateResourceBuffer(nullptr, uniBufize, m_scene.meshInfo_uniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return false;

	return true;
}

bool CRasterRender::CreateSkybox(std::vector<Buffer>& p_stgbfrList, VkCommandBuffer& p_cmdbfr)
{
	std::string cubemap_path[6]{
		g_AssetPath + "/skybox/Daylight_Box_Pieces/f.png"
	,	g_AssetPath + "/skybox/Daylight_Box_Pieces//b.png"
	,	g_AssetPath + "/skybox/Daylight_Box_Pieces/t.png"
	,	g_AssetPath + "/skybox/Daylight_Box_Pieces/bt.png"
	,	g_AssetPath + "/skybox/Daylight_Box_Pieces/l.png"
	,	g_AssetPath + "/skybox/Daylight_Box_Pieces/r.png"
	};

	std::vector<unsigned char> megaStg_data;
	ImageRaw cubemap_raw[6];
	for (int i = 0; i < 6; i++)
	{
		if (!LoadRawImage(cubemap_path[i].c_str(), cubemap_raw[i]))
			return false;

		int size = cubemap_raw[i].width * cubemap_raw[i].height * cubemap_raw[i].channels;
		megaStg_data.insert(megaStg_data.end(), cubemap_raw[i].raw, cubemap_raw[i].raw + size);
	}

	Buffer tex_stgbfr;
	if (!CreateResourceBuffer(megaStg_data.data(), megaStg_data.size(), tex_stgbfr, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return false;

	m_scene.skybox_cubemap.descInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_scene.skybox_cubemap.descInfo.sampler		= m_samplers[SamplerId::s_Linear].descInfo.sampler;
	m_scene.skybox_cubemap.format				= VK_FORMAT_R8G8B8A8_UNORM;
	m_scene.skybox_cubemap.width				= cubemap_raw[0].width;
	m_scene.skybox_cubemap.height				= cubemap_raw[0].height;
	m_scene.skybox_cubemap.layerCount			= 6;
	m_scene.skybox_cubemap.bufOffset			= cubemap_raw[0].width * cubemap_raw[0].height * cubemap_raw[0].channels;
	m_scene.skybox_cubemap.viewType				= VK_IMAGE_VIEW_TYPE_CUBE;

	VkImageCreateInfo imgInfo	= CVulkanCore::ImageCreateInfo();
	imgInfo.extent.width		= cubemap_raw[0].width;	// assuming all the loaded cubemap images have same width
	imgInfo.extent.height		= cubemap_raw[0].height;	// assuming all the loaded cubemap images have same height
	imgInfo.arrayLayers			= 6;
	imgInfo.flags				= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	imgInfo.usage				= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imgInfo.format				= VK_FORMAT_R8G8B8A8_UNORM;

	if (!CreateTexture(tex_stgbfr, m_scene.skybox_cubemap, imgInfo, p_cmdbfr))
		return false;

	p_stgbfrList.push_back(tex_stgbfr);

	SceneRaw sceneraw;
	ObjLoadData loadData{};
	loadData.flipUV			= false;
	loadData.loadMeshOnly	= true;
	
	std::string skybox_obj_path = g_AssetPath + "/skybox/skybox.obj";
	if (!LoadObj(skybox_obj_path.c_str(), sceneraw, loadData))
		return false;

	MeshRaw meshraw = sceneraw.meshList[0];
	RenderMesh mesh;
	mesh.mesh_id		= MeshType::mt_Skybox;
	mesh.modelMatrix	= nm::float4x4::identity();

	{
		Buffer vertexStg;
		if (!CreateResourceBuffer(meshraw.vertexList.data(), sizeof(Vertex) * meshraw.vertexList.size(), vertexStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		p_stgbfrList.push_back(vertexStg);

		if (!CreateLoadedBuffer(vertexStg, mesh.vertexBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, p_cmdbfr))
			return false;
	}

	{
		Buffer indxStg;
		if (!CreateResourceBuffer(meshraw.indicesList.data(), sizeof(uint32_t) * meshraw.indicesList.size(), indxStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		p_stgbfrList.push_back(indxStg);

		if (!CreateLoadedBuffer(indxStg, mesh.indexBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, p_cmdbfr))
			return false;
	}

	m_scene.meshes.push_back(mesh);

	sceneraw.materialsList.clear();
	sceneraw.meshList.clear();
	sceneraw.textureList.clear();
	for (int i = 0; i < 6; i++)
		FreeRawImage(cubemap_raw[i]);

	return true;
}

bool CRasterRender::CreateScene(std::string* p_paths, bool* p_flipVList, unsigned int p_count)
{
	std::vector<Buffer> staginBufferList;

	VkCommandBuffer transferCmdBfr;
	if (!CreateCommandBuffers(m_vkCmdPool, &transferCmdBfr, 1))
		return false;
	if (!BeginCommandBuffer(transferCmdBfr, "Scene Loading"))
		return false;

	{	
		if (!CreateSkybox(staginBufferList, transferCmdBfr))
			return false;
	}

	// load default texture to compensate for replace textures
	{
		ImageRaw tex;
		if (!LoadRawImage((g_AssetPath + "/tex_not_found.png").c_str(), tex))
			return false;

		size_t texSize = tex.width * tex.height * tex.channels;

		Buffer texstg;
		if (!CreateResourceBuffer(tex.raw, texSize, texstg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		staginBufferList.push_back(texstg);

		Image img;
		img.format		= VK_FORMAT_B8G8R8A8_SRGB;
		img.width		= tex.width;
		img.height		= tex.height;
		img.viewType	= VK_IMAGE_VIEW_TYPE_2D;

		VkImageCreateInfo imgCrtInfo = CVulkanCore::ImageCreateInfo();
		imgCrtInfo.extent.width		= tex.width;
		imgCrtInfo.extent.height	= tex.height;
		imgCrtInfo.format			= VK_FORMAT_B8G8R8A8_SRGB;
		imgCrtInfo.usage			= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (!CreateTexture(texstg, img, imgCrtInfo, transferCmdBfr))
			return false;

		m_scene.default_tex = img;

		FreeRawImage(tex);
	}

	SceneRaw sceneraw;
	for (unsigned int i = 0; i < p_count; i++)
	{
		ObjLoadData loadData{};
		loadData.flipUV = p_flipVList[i];
		loadData.loadMeshOnly = false;

		if (!LoadGltf(p_paths[i].c_str(), sceneraw, loadData))
			return false;
	}
	
	for (auto& meshraw : sceneraw.meshList)
	{
		RenderMesh mesh;
		mesh.mesh_id			= (uint32_t)m_scene.meshes.size();
		mesh.modelMatrix		= meshraw.transform;
		mesh.submeshes.resize(meshraw.submeshes.size());
		std::copy(meshraw.submeshes.begin(), meshraw.submeshes.end(), mesh.submeshes.begin());

		{
			Buffer vertexStg;
			if (!CreateResourceBuffer(meshraw.vertexList.data(), sizeof(Vertex) * meshraw.vertexList.size(), vertexStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				return false;

			staginBufferList.push_back(vertexStg);

			if (!CreateLoadedBuffer(vertexStg, mesh.vertexBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, transferCmdBfr))
				return false;
		}

		{
			Buffer indxStg;
			if (!CreateResourceBuffer(meshraw.indicesList.data(), sizeof(uint32_t) * meshraw.indicesList.size(), indxStg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				return false;

			staginBufferList.push_back(indxStg);

			if (!CreateLoadedBuffer(indxStg, mesh.indexBuffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, transferCmdBfr))
				return false;
		}
		m_scene.meshes.push_back(mesh);
	}
	
	for (const auto& tex : sceneraw.textureList)
	{
		Image img;
		{
			if (tex.raw != nullptr)
			{
				size_t texSize = tex.width * tex.height * tex.channels;

				Buffer texstg;
				if (!CreateResourceBuffer(tex.raw, texSize, texstg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
					return false;

				staginBufferList.push_back(texstg);

				img.format		= VK_FORMAT_R8G8B8A8_UNORM;
				img.width		= tex.width;
				img.height		= tex.height;
				img.viewType	= VK_IMAGE_VIEW_TYPE_2D;

				VkImageCreateInfo imgCrtInfo	= CVulkanCore::ImageCreateInfo();
				imgCrtInfo.extent.width			= tex.width;
				imgCrtInfo.extent.height		= tex.height;
				imgCrtInfo.format				= img.format;
				imgCrtInfo.usage				= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

				if (!CreateTexture(texstg, img, imgCrtInfo, transferCmdBfr))
					return false;
			}
			else
			{
				img = m_scene.default_tex;

			}
		}
		
		m_scene.textures.push_back(img);

	}

	// storage buffer for materials
	{
		Buffer mat_Stg;
		if (!CreateResourceBuffer(sceneraw.materialsList.data(), sizeof(Material) * sceneraw.materialsList.size(), mat_Stg, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		staginBufferList.push_back(mat_Stg);

		if (!CreateLoadedBuffer(mat_Stg, m_scene.material_storage, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, transferCmdBfr))
			return false;
	}

	if (!EndCommandBuffer(transferCmdBfr))
		return false;

	VkPipelineStageFlags waitstage{ VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	VkSubmitInfo submitInfo{};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &transferCmdBfr;
	submitInfo.pSignalSemaphores	= nullptr;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pWaitSemaphores		= nullptr;
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.pWaitDstStageMask	= &waitstage;
	if (!SubmitCommandbuffer(m_vkQueue[0], &submitInfo, 1))
		return false;

	if (!WaitToFinish(m_vkQueue[0]))
		return false;

	for (auto& stg : staginBufferList)
		DestroyBuffer(stg.descInfo.buffer);
	
	// can binary dump in future to optimised format for faster binary loading
	sceneraw.meshList.clear();
	for(auto& tex : sceneraw.textureList)
		FreeRawImage(tex);

	sceneraw.textureList.clear();

	if (!CreateMeshUniformBuffer())
		return false;

	if (!CreateSceneDescriptors())
		return false;

	return true;
}

void CRasterRender::ClearFixedResource()
{
	for (auto& buf : m_fixedBufferList)
		DestroyBuffer(buf.descInfo.buffer);

	for (auto& rt : m_renderTargets)
		DestroyImage(rt.image);

	for (auto& samp : m_samplers)
		DestroySampler(samp.descInfo.sampler);

	m_fixedBufferList.clear();
	m_renderTargets.clear();
	m_samplers.clear();
}