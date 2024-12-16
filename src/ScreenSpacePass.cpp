#include "ScreenSpacePass.h"
#include "core/RandGen.h"

CSSAOComputePass::CSSAOComputePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_kernelSize(64.0f)
	, m_kernelRadius(0.5f)
	, m_bias(0.015f)
	, m_noiseScale(nm::float2((float)p_rhi->GetRenderWidth() / 4.0f, (float)p_rhi->GetRenderHeight() / 4.0f))
{}

CSSAOComputePass::~CSSAOComputePass()
{
}

bool CSSAOComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath /"shaders/spirv/SSAO.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "SSAOComputePipeline"))
		return false;

	return true;
}

bool CSSAOComputePass::Update(UpdateData* p_updateData)
{
	p_updateData->uniformData->biasSSAO = m_bias;
	p_updateData->uniformData->enableSSAO = m_isEnabled;
	p_updateData->uniformData->ssaoKernelSize = m_kernelSize;
	p_updateData->uniformData->ssaoNoiseScale = m_noiseScale;
	p_updateData->uniformData->ssaoRadius = m_kernelRadius;

	return true;
}

bool CSSAOComputePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x									= m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y									= m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO"));

	p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSAO_Blur);

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSAOComputePass::Show(CVulkanRHI* p_rhi)
{
	bool ssaoNode = ImGui::TreeNode("SSAO");
	ImGui::SameLine(75);
	ImGui::Checkbox(" ", &m_isEnabled);

	if (ssaoNode)
	{
		ImGui::SliderFloat("SSAO Bias", &m_bias, 0.00f, 0.1f);
		ImGui::InputFloat2("Noise Scale", &m_noiseScale[0]);
		ImGui::InputFloat("Kernel Size", &m_kernelSize);
		ImGui::InputFloat("Kernel Radius", &m_kernelRadius);
		ImGui::TreePop();
	}
}

CSSAOBlurPass::CSSAOBlurPass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{}

CSSAOBlurPass::~CSSAOBlurPass()
{}

bool CSSAOBlurPass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoBlurShaderpaths{};
	ssaoBlurShaderpaths.shaderpath_compute = g_EnginePath /"shaders/spirv/SSAOBlur.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoBlurShaderpaths, m_pipeline, "SSAOBlurComputePipeline"))
		return false;

	return true;
}

bool CSSAOBlurPass::Update(UpdateData*)
{
	return true;
}

bool CSSAOBlurPass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId											= p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr						= p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc					= p_renderData->primaryDescriptors;

	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Compute SSAO Blur"));

	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);

	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

CSSRComputePass::CSSRComputePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
	, m_maxDistance(5.0f)
	, m_resolution(1.0f)
	, m_thickness(0.11f)
	, m_steps(2)
{}

CSSRComputePass::~CSSRComputePass()
{
}

bool CSSRComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/SSR.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "SSRComputePipeline"))
		return false;
	
	return true;
}

bool CSSRComputePass::Update(UpdateData* p_updateData)
{
	p_updateData->uniformData->ssrEnable		= IsEnabled();
	p_updateData->uniformData->ssrMaxDistance	= m_maxDistance;
	p_updateData->uniformData->ssrResolution	= m_resolution;
	p_updateData->uniformData->ssrThickness		= m_thickness;
	p_updateData->uniformData->ssrSteps			= m_steps;

	return true;
}

bool CSSRComputePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	const CScene* scene = p_renderData->loadedAssets->GetScene();
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;

	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "SSR"));
	m_rhi->InsertMarker(cmdBfr, "Compute");
	{
		p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);

		vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
		vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Scene, 1, scene->GetDescriptorSet(scId), 0, nullptr);
		vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	}

	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

	return true;
}

void CSSRComputePass::Show(CVulkanRHI* p_rhi)
{
	bool ssrNode = ImGui::TreeNode("SSR");
	ImGui::SameLine(75);
	bool isSSREnabled = IsEnabled();
	ImGui::Checkbox(" ", &isSSREnabled);
	Enable(isSSREnabled);

	if (ssrNode)
	{
		ImGui::Indent();
		ImGui::SliderFloat("Max Distance", &m_maxDistance, 0.0f, 50.0f);
		ImGui::SliderFloat("Resolution", &m_resolution, 0.0f, 1.0f);
		ImGui::SliderFloat("Steps", &m_steps, 1, 50);
		ImGui::SliderFloat("Thickness", &m_thickness, 0.0f, 5.00f);

		ImGui::Unindent();
		ImGui::TreePop();
	}
}

CCopyComputePass::CCopyComputePass(CVulkanRHI* p_rhi)
	: CComputePass(p_rhi)
{}

CCopyComputePass::~CCopyComputePass()
{
}

bool CCopyComputePass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
	CVulkanRHI::ShaderPaths ssaoShaderpaths{};
	ssaoShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/Copy.comp.spv";
	m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
	if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "CopyComputePipeline"))
		return false;

	return true;
}

bool CCopyComputePass::Update(UpdateData*)
{
	return true;
}

bool CCopyComputePass::Dispatch(RenderData* p_renderData)
{
	uint32_t scId = p_renderData->scIdx;
	CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
	const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
	uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
	uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;
	
	RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Copy Compute"));

	p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSReflection);
	
	vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
	vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
	
	RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));
	
	return true;
}

CFidelityFXSSSRPass::CFidelityFXSSSRPass(CVulkanRHI* p_rhi)
	: CStaticRenderPass(p_rhi)
	, CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
{
}

CFidelityFXSSSRPass::~CFidelityFXSSSRPass()
{
}

bool CFidelityFXSSSRPass::CreatePipeline(CVulkanRHI::Pipeline)
{
	return true;
}

bool CFidelityFXSSSRPass::Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline)
{
	RETURN_FALSE_IF_FALSE(InitFfxContext());
	RETURN_FALSE_IF_FALSE(InitSSSRContext(p_renderData));

	return true;
}

bool CFidelityFXSSSRPass::Update(UpdateData*)
{
	return false;
}

bool CFidelityFXSSSRPass::Render(RenderData* p_renderData)
{

	FfxSssrDispatchDescription dispatchParam = {};
	dispatchParam.commandList = p_renderData->cmdBfr;
	dispatchParam.color = GetColorResource(p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::RenderTargetId::rt_PrimaryColor));
	dispatchParam.depth = GetDepthResource(p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::RenderTargetId::rt_PrimaryDepth));

	return false;
}

void CFidelityFXSSSRPass::Show(CVulkanRHI* p_rhi)
{
}

FfxResource CFidelityFXSSSRPass::GetColorResource(const CVulkanRHI::Image& p_Image)
{
	FfxResource colorResource{};

	FfxResourceDescription desc = {};
	desc.type		= FFX_RESOURCE_TYPE_TEXTURE2D;
	desc.format		= FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
	desc.width		= p_Image.width;
	desc.height		= p_Image.height;
	desc.depth		= 1.0f;
	desc.mipCount	= 1;
	desc.flags		= FFX_RESOURCE_FLAGS_NONE;
	desc.usage		= FFX_RESOURCE_USAGE_UAV;	

	colorResource.description = desc;
	colorResource.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;

	return colorResource;
}

FfxResource CFidelityFXSSSRPass::GetDepthResource(const CVulkanRHI::Image& p_Image)
{
	FfxResource depthResource{};

	FfxResourceDescription desc = {};
	desc.type		= FFX_RESOURCE_TYPE_TEXTURE2D;
	//desc.format		= VK_FORMAT_D32_SFLOAT_S8_UINT;
	desc.width		= p_Image.width;
	desc.height		= p_Image.height;
	desc.depth		= 1.0f;
	desc.mipCount	= 1;
	desc.flags		= FFX_RESOURCE_FLAGS_NONE;
	desc.usage		= FFX_RESOURCE_USAGE_UAV;

	depthResource.description = desc;
	depthResource.state = FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ;

	return depthResource;
}

bool CFidelityFXSSSRPass::InitFfxContext()
{
	const size_t scratchBufferSize = ffxGetScratchMemorySizeVK(m_rhi->GetPhysicalDevice(), FFX_SSSR_CONTEXT_COUNT);
	void* scratchBuffer = calloc(scratchBufferSize, 1);

	// Create a scratch space for SSSR. For now only populating the interface
	FfxErrorCode errorCode = ffxGetInterfaceVK(m_ffxInterface, m_rhi->GetDevice(), scratchBuffer, scratchBufferSize, FFX_SSSR_CONTEXT_COUNT);
	if (errorCode != FfxErrorCodes::FFX_OK)
	{
		std::cerr << "Failed to Create AMD::FFX::SSSR Scratch Space. Error Code  " << errorCode << std::endl;
		return false;
	}

	// We can check for FFX_SDK_MAKE_VERSION for specific feature support but we will skip that for now

	// Next populate the constant buffer allocator; for now we will skip this. 
	// I wonder if I can fully rely on Ffx sdk for constant allocation as well
	// Commenting out for now
	// ffxInterface->fpRegisterConstantBufferAllocator(ffxInterface, );

	return true;

}

bool CFidelityFXSSSRPass::InitSSSRContext(RenderData* p_renderData)
{
	CVulkanRHI::Image normalRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::RenderTargetId::rt_Normal);

	FfxSssrContext sssrContext;
	FfxSssrContextDescription sssrDescription{};
	sssrDescription.flags = 0; // not inverting the depth
	sssrDescription.renderSize = FfxDimensions2D{ m_rhi->GetRenderWidth(), m_rhi->GetRenderHeight() };
	sssrDescription.backendInterface = *m_ffxInterface;
	sssrDescription.normalsHistoryBufferFormat = FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT; // hardcoding this for now because Ffx has its own format type
	FfxErrorCode errorCode = ffxSssrContextCreate(&sssrContext, &sssrDescription);
	if (errorCode != FfxErrorCodes::FFX_OK)
	{
		std::cerr << "Failed to Create AMD::FFX::SSSR Context. Error Code  " << errorCode << std::endl;
		return false;
	}

	return true;
}
