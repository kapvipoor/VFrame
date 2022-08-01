#include "SSAOPass.h"
#include "core/RandGen.h"

CSSAOComputePass::CSSAOComputePass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
	m_frameBuffer.resize(1);
}

CSSAOComputePass::~CSSAOComputePass()
{
}

bool CSSAOComputePass::CreateRenderpass(RenderData* p_renderData)
{
	// compute pass, not Renderpass to create
	return true;
}

bool CSSAOComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath + "/shaders/spirv/SSAO.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline))
		return false;

	return true;
}

bool CSSAOComputePass::Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline)
{
	RETURN_FALSE_IF_FALSE(CreateRenderpass(p_renderData));
	RETURN_FALSE_IF_FALSE(CreatePipeline(p_pipeline));
	return true;
}

bool CSSAOComputePass::Update(UpdateData*)
{
	return true;
}

bool CSSAOComputePass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x									= m_rhi->GetRenderWidth() / 8;
	uint32_t dispatchDim_y									= m_rhi->GetRenderHeight() / 8;

	if (!m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO"))
		return false;

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CSSAOComputePass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CSSAOComputePass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription					= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription						= m_pipeline.vertexInBinding;
}

CSSAOBlurPass::CSSAOBlurPass(CVulkanRHI* p_rhi)
	: CPass(p_rhi)
{
}

CSSAOBlurPass::~CSSAOBlurPass()
{
}

bool CSSAOBlurPass::CreateRenderpass(RenderData* p_renderData)
{
	// compute pass, not Renderpass to create
	return true;
}

bool CSSAOBlurPass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoBlurShaderpaths{};
	ssaoBlurShaderpaths.shaderpath_compute = g_EnginePath + "/shaders/spirv/SSAOBlur.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoBlurShaderpaths, m_pipeline))
		return false;

	return true;
}

bool CSSAOBlurPass::Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline)
{
	RETURN_FALSE_IF_FALSE(CreateRenderpass(p_renderData));
	RETURN_FALSE_IF_FALSE(CreatePipeline(p_pipeline));
	return true;
}

bool CSSAOBlurPass::Update(UpdateData*)
{
	return true;
}

bool CSSAOBlurPass::Render(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;

	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / 8;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / 8;

	if (!m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO Blur"))
		return false;

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	m_rhi->EndCommandBuffer(cmdBfr);

	return true;
}

void CSSAOBlurPass::Destroy()
{
	m_rhi->DestroyPipeline(m_pipeline);
}

void CSSAOBlurPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
	p_vertexBinding.attributeDescription			= m_pipeline.vertexAttributeDesc;
	p_vertexBinding.bindingDescription				= m_pipeline.vertexInBinding;
}