#include "ShadowPass.h"

CStaticShadowPrepass::CStaticShadowPrepass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CStaticShadowPrepass::~CStaticShadowPrepass()
{
}

bool CStaticShadowPrepass::CreateRenderpass(RenderData* p_renderData)
{
	CVulkanRHI::Image renderTarget			= p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_LightDepth);
	CVulkanRHI::Renderpass* renderpass		= &m_pipeline.renderpassData; 

	renderpass->AttachDepth(
		renderTarget.format,		VK_ATTACHMENT_LOAD_OP_CLEAR,						VK_ATTACHMENT_STORE_OP_STORE,
									VK_IMAGE_LAYOUT_UNDEFINED,							VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,	
									VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		
	renderpass->framebufferWidth							= renderTarget.width;
	renderpass->framebufferHeight							= renderTarget.height;
	if (!m_rhi->CreateRenderpass(*renderpass))
		return false;

	std::vector<VkImageView> attachments(1, VkImageView{});
	attachments[0]											= renderTarget.descInfo.imageView;
	if (!m_rhi->CreateFramebuffer(renderpass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderpass->framebufferWidth, renderpass->framebufferHeight))
		return false;

	return true;
}

bool CStaticShadowPrepass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	VkVertexInputBindingDescription vertexInputBinding		= {};
	vertexInputBinding.binding								= 0;
	vertexInputBinding.stride								= sizeof(Vertex);
	vertexInputBinding.inputRate							= VK_VERTEX_INPUT_RATE_VERTEX;

	//	layout (location = 0) in vec3 pos;
	//	layout (location = 1) in vec3 normal;
	//	layout (location = 2) in vec2 uv;
	//	layout (location = 3) in vec3 tangent;
	std::vector<VkVertexInputAttributeDescription> vertexAttributs;
	// Attribute location 0: pos
	VkVertexInputAttributeDescription attribDesc{};
	attribDesc.binding										= 0;
	attribDesc.location										= 0;
	attribDesc.format										= VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset										= offsetof(Vertex, pos);
	vertexAttributs.push_back(attribDesc);

	// Attribute location 1: normal
	attribDesc.binding										= 0;
	attribDesc.location										= 1;
	attribDesc.format										= VK_FORMAT_R32G32B32_SFLOAT;
	attribDesc.offset										= offsetof(Vertex, normal);
	vertexAttributs.push_back(attribDesc);

	// Attribute location 2: uv
	attribDesc.binding	= 0;
	attribDesc.location										= 2;
	attribDesc.format										= VK_FORMAT_R32G32_SFLOAT;
	attribDesc.offset										= offsetof(Vertex, uv);
	vertexAttributs.push_back(attribDesc);

	// Attribute location 3: tangent
	attribDesc.binding	= 0;
	attribDesc.location										= 3;
	attribDesc.format										= VK_FORMAT_R32G32B32A32_SFLOAT;
	attribDesc.offset										= offsetof(Vertex, tangent);
	vertexAttributs.push_back(attribDesc);

	CVulkanRHI::ShaderPaths shadowPassShaderpaths{};
	shadowPassShaderpaths.shaderpath_vertex					= g_EnginePath + "/shaders/spirv/LightDepthPrepass.vert.spv";
	m_pipeline.pipeLayout									= p_Pipeline.pipeLayout;
	m_pipeline.vertexInBinding								= vertexInputBinding;
	m_pipeline.vertexAttributeDesc							= vertexAttributs;
	m_pipeline.cullMode										= VK_CULL_MODE_BACK_BIT;
	m_pipeline.enableDepthTest								= true;
	m_pipeline.enableDepthWrite								= true;
	if (!m_rhi->CreateGraphicsPipeline(shadowPassShaderpaths, m_pipeline))
	{
		std::cout << "Error Creating Static Shadow Pipeline" << std::endl;
		return false;
	}

	return true;
}

bool CStaticShadowPrepass::Update(UpdateData*)
{
	return true;
}

bool CStaticShadowPrepass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	CVulkanRHI::Renderpass renderPass						= m_pipeline.renderpassData;
	const CScene* scene										= p_renderData->loadedAssets->GetScene();
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(p_renderData->cmdBfr, "Shadow Map Generation"));
	m_rhi->BeginRenderpass(m_frameBuffer[0], renderPass, p_renderData->cmdBfr);

	uint32_t fbWidth										= renderPass.framebufferWidth;
	uint32_t fbHeight										= renderPass.framebufferHeight;
	m_rhi->SetViewport(p_renderData->cmdBfr, 0.0f, 1.0f, (float)fbWidth, (float)fbHeight);
	m_rhi->SetScissors(p_renderData->cmdBfr, 0, 0, fbWidth, fbHeight);

	vkCmdBindPipeline(p_renderData->cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(p_renderData->cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdBindDescriptorSets(p_renderData->cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(), 0, nullptr);

	// Bind Index and Vertices buffers
	VkDeviceSize offsets[1] = { 0 };
	for (unsigned int i = CScene::MeshType::mt_Scene; i < scene->GetRenderableMeshCount(); i++)
	{
		const CRenderableMesh* mesh							= scene->GetRenderableMesh(i);
		const CVulkanRHI::Buffer* vertex					= mesh->GetVertexBuffer();
		const CVulkanRHI::Buffer* index						= mesh->GetIndexBuffer();

		vkCmdBindVertexBuffers(p_renderData->cmdBfr, 0, 1, &vertex->descInfo.buffer, offsets);
		vkCmdBindIndexBuffer(p_renderData->cmdBfr, index->descInfo.buffer, 0, VK_INDEX_TYPE_UINT32);

		for (uint32_t j = 0; j < mesh->GetSubmeshCount(); j++)
		{
			const SubMesh* submesh			= mesh->GetSubmesh(j);
			VkPipelineStageFlags vertex_frag				= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			CScene::MeshPushConst pc{ mesh->GetMeshId(), submesh->materialId};
			
			vkCmdPushConstants(p_renderData->cmdBfr, m_pipeline.pipeLayout, vertex_frag, 0, sizeof(CScene::MeshPushConst), (void*)&pc);

			//uint32_t count = (uint32_t)mesh.indexBuffer.descInfo.range / sizeof(uint32_t);
			vkCmdDrawIndexed(p_renderData->cmdBfr, submesh->indexCount, 1, submesh->firstIndex, 0, 1);
		}
	}

	m_rhi->EndRenderPass(p_renderData->cmdBfr);
	m_rhi->EndCommandBuffer(p_renderData->cmdBfr);

	return true;
}

void CStaticShadowPrepass::Destroy()
{
	m_rhi->DestroyFramebuffer(m_frameBuffer[0]);
	m_rhi->DestroyRenderpass(m_pipeline.renderpassData.renderpass);
	m_rhi->DestroyPipeline(m_pipeline);
}

void CStaticShadowPrepass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}
