#pragma once

#include "PostProcessingPasses.h"

CToneMapPass::CToneMapPass(CVulkanRHI* p_rhi)
    : CPass(p_rhi)
    , CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
    , m_toneMapper(ToneMapper::AMD)
    , m_exposure(1.0f)
{
    m_frameBuffer.resize(FRAME_BUFFER_COUNT);
}

CToneMapPass::~CToneMapPass()
{
}

bool CToneMapPass::CreateRenderpass(RenderData* p_renderData)
{
    CVulkanRHI::Renderpass* renderpass = &m_pipeline.renderpassData;

    renderpass->AttachColor(VK_FORMAT_B8G8R8A8_UNORM, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    if (!m_rhi->CreateRenderpass(*renderpass))
        return false;

    renderpass->framebufferWidth = m_rhi->GetRenderWidth();
    renderpass->framebufferHeight = m_rhi->GetRenderHeight();
    std::vector<VkImageView> attachments(1, VkImageView{});
    attachments[0] = m_rhi->GetSCImageView(0);
    if (!m_rhi->CreateFramebuffer(renderpass->renderpass, m_frameBuffer[0], attachments.data(), (uint32_t)attachments.size(), renderpass->framebufferWidth, renderpass->framebufferHeight))
        return false;

    attachments[0] = m_rhi->GetSCImageView(1);
    if (!m_rhi->CreateFramebuffer(renderpass->renderpass, m_frameBuffer[1], attachments.data(), (uint32_t)attachments.size(), renderpass->framebufferWidth, renderpass->framebufferHeight))
        return false;

    return true;
}

bool CToneMapPass::CreatePipeline(CVulkanCore::Pipeline p_Pipeline)
{
    CVulkanRHI::ShaderPaths uiShaderpaths{};
    uiShaderpaths.shaderpath_vertex = g_EnginePath / "shaders/spirv/FulllScreenQuad.vert.spv";
    uiShaderpaths.shaderpath_fragment = g_EnginePath / "shaders/spirv/ToneMap.frag.spv";
    m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
    m_pipeline.cullMode = VK_CULL_MODE_FRONT_BIT;
    m_pipeline.enableBlending = false;
    m_pipeline.enableDepthTest = false;
    m_pipeline.enableDepthWrite = false;
    if (!m_rhi->CreateGraphicsPipeline(uiShaderpaths, m_pipeline, "TonemapGfxPipeline"))
    {
        std::cout << "Error Creating Tone Mapping Pipeline" << std::endl;
        return false;
    }

    return true;
}

bool CToneMapPass::Update(UpdateData* p_updateData)
{
    return true;
}

bool CToneMapPass::Render(RenderData* p_renderData)
{
    uint32_t scId = p_renderData->scIdx;
    CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
    CVulkanRHI::Renderpass renderPass = m_pipeline.renderpassData;
    const CRenderableUI* ui = p_renderData->loadedAssets->GetUI();
    const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;

    RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "Tone Mapping"));

    m_rhi->SetClearColorValue(renderPass, 0, VkClearColorValue{ 0.0f, 0.0f, 0.0f, 1.00f });
    m_rhi->BeginRenderpass(m_frameBuffer[p_renderData->scIdx], renderPass, cmdBfr);

    m_rhi->SetViewport(cmdBfr, 0.0f, 1.0f, (float)renderPass.framebufferWidth, (float)renderPass.framebufferHeight);
    m_rhi->SetScissors(cmdBfr, 0, 0, renderPass.framebufferWidth, renderPass.framebufferHeight);

    vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeline);
    vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
    vkCmdDraw(cmdBfr, 3, 1, 0, 0);
    m_rhi->EndRenderPass(cmdBfr);

    m_rhi->InsertMarker(cmdBfr, "Resources Copy/Clear");

    CVulkanRHI::Image colorRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
    CVulkanRHI::Image prevColorlRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Prev_PrimaryColor);
    m_rhi->CopyImage(cmdBfr, colorRT, prevColorlRT);

    VkClearValue clear{};
    clear.color = { 0.0, 0.0, 0.0, 1.0 };
    CVulkanRHI::Image ssrRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_SSReflection);
    m_rhi->ClearImage(cmdBfr, ssrRT, clear);

    m_rhi->EndCommandBuffer(cmdBfr);

    return true;
}

void CToneMapPass::Destroy()
{
    m_rhi->DestroyFramebuffer(m_frameBuffer[0]);
    m_rhi->DestroyFramebuffer(m_frameBuffer[1]);
    m_rhi->DestroyRenderpass(m_pipeline.renderpassData.renderpass);
    m_rhi->DestroyPipeline(m_pipeline);
}

void CToneMapPass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
    p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
    p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}

void CToneMapPass::Show(CVulkanRHI* p_rhi)
{
    bool toneMappingNode = ImGui::TreeNode("Tone Mapping");
    if (toneMappingNode)
    {
        ImGui::Indent();
      
        const char* items[] = {"None", "Reinhard", "AMD" };
        int tonemappingMode = (int)m_toneMapper;
        ImGui::ListBox("Tone Mapper", &tonemappingMode, items, IM_ARRAYSIZE(items), 3);
        m_toneMapper = (ToneMapper)tonemappingMode;

        ImGui::SliderFloat("Exposure", &m_exposure, 0.1f, 5.0f);

        ImGui::Unindent();

        ImGui::TreePop();
    }
}

CTAAComputePass::CTAAComputePass(CVulkanRHI* p_rhi)
    : CPass(p_rhi)
    , CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
    , m_activeJitterMode(CJitterHelper::JitterMode::Hammersley16x)
    , m_resolveWeight(0.93f)
    , m_useMotionVectors(false)
    , m_flickerCorrectionMode(FlickerCorrection::None)
    , m_reprojectionFilter(ReprojectionFilter::Standard)
{
    m_frameBuffer.resize(1);
    //m_isEnabled = false; // for now disabling it by default
	m_jitterHelper = new CJitterHelper(m_activeJitterMode);
}

CTAAComputePass::~CTAAComputePass()
{
	delete m_jitterHelper;
}

bool CTAAComputePass::CreateRenderpass(RenderData*)
{
    // compute pass, no render pass to create
    return true;
}

bool CTAAComputePass::CreatePipeline(CVulkanRHI::Pipeline p_Pipeline)
{
    CVulkanRHI::ShaderPaths ssaoShaderpaths{};
    ssaoShaderpaths.shaderpath_compute = g_EnginePath / "shaders/spirv/TAA.comp.spv";
    m_pipeline.pipeLayout = p_Pipeline.pipeLayout;
    if (!m_rhi->CreateComputePipeline(ssaoShaderpaths, m_pipeline, "TAAComputePipeline"))
        return false;

    return true;
}

bool CTAAComputePass::Update(UpdateData*)
{
    return true;
}

bool CTAAComputePass::Render(RenderData* p_renderData)
{
    uint32_t scId = p_renderData->scIdx;
    CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
    const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
    uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
    uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;
    
    if (!m_rhi->BeginCommandBuffer(cmdBfr, "Compute TAA"))
        return false;
    
    vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
    vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
    vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
    
    m_rhi->EndCommandBuffer(cmdBfr);

    return true;
}

void CTAAComputePass::Destroy()
{
    m_rhi->DestroyPipeline(m_pipeline);
}

void CTAAComputePass::GetVertexBindingInUse(CVulkanCore::VertexBinding& p_vertexBinding)
{
    p_vertexBinding.attributeDescription = m_pipeline.vertexAttributeDesc;
    p_vertexBinding.bindingDescription = m_pipeline.vertexInBinding;
}

void CTAAComputePass::Show(CVulkanRHI* p_rhi)
{
    bool taaNode = ImGui::TreeNode("TAA");
    ImGui::SameLine(75);
    bool isTAAEnabled = IsEnabled();
    ImGui::Checkbox(" ", &isTAAEnabled);
    Enable(isTAAEnabled);

    if (m_isEnabled == false)
    {
        m_jitterHelper->m_jitterMode = CJitterHelper::JitterMode::None;
    }
    else
    {
        m_jitterHelper->m_jitterMode = m_activeJitterMode;
    }
    if (taaNode)
    {
        ImGui::Indent();
        if (ImGui::TreeNode("Jitter"))
        {
            ImGui::InputFloat2("Jitter Offset", &m_jitterHelper->m_jitterOffset[0]);

            ImGui::SliderFloat("Jitter Scale", &m_jitterHelper->m_jitterScale, 0.00f, 50.0f);

            const char* items[] = { "None", "Uniform2x", "Hammersley4x", "Hammersley8x", "Hammersley16x" };
            int jitMod = (int)m_jitterHelper->m_jitterMode;
            ImGui::ListBox("Jitter Mode", &jitMod, items, IM_ARRAYSIZE(items), 3);
            m_jitterHelper->m_jitterMode = (CJitterHelper::JitterMode)jitMod;
            
            if(m_isEnabled)
                m_activeJitterMode = m_jitterHelper->m_jitterMode;

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Reproject"))
        {
            ImGui::Checkbox("Use Motion Vectors ", &m_useMotionVectors);

            const char* items[] = { "Standard", "CatmullRom"};
            int reprojFilter = (int)m_reprojectionFilter;
            ImGui::ListBox("Reprojection Filter", &reprojFilter, items, IM_ARRAYSIZE(items), 2);
            m_reprojectionFilter = (ReprojectionFilter)reprojFilter;

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Resolve"))
        {
            ImGui::SliderFloat("Resolve Weight", &m_resolveWeight, 0.0f, 1.0f);

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Flicker Correction"))
        {
            const char* items[] = { "None", "Log Weighing", "Luminance Weighing"};
            int filckerCorrectionMode = (int)m_flickerCorrectionMode;
            ImGui::ListBox("Correction Algorithm", &filckerCorrectionMode, items, IM_ARRAYSIZE(items), 3);
            m_flickerCorrectionMode = (FlickerCorrection)filckerCorrectionMode;

            ImGui::TreePop();
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

CTAAComputePass::CJitterHelper::CJitterHelper(JitterMode p_jitterMode)
    : m_jitterMode(p_jitterMode)
    , m_jitterScale(0.03f)
    , m_jitterOffset(0.0f)
{
}

nm::float4x4 CTAAComputePass::CJitterHelper::ComputeJitter(const uint64_t p_frameCount)
{
    if (m_jitterMode == JitterMode::None)
    {
        return nm::float4x4::identity();
    }
    else if (m_jitterMode == JitterMode::Uniform2x)
    {
        float offset = p_frameCount % 2 == 0 ? -0.5f : 0.5f;
        m_jitterOffset = nm::float2(offset);
    }
    else if (m_jitterMode == JitterMode::Hammersley4x)
    {
        uint64_t idx = p_frameCount % 4;
        m_jitterOffset = Hammersley2D(idx, 4) * 2.0f - nm::float2(1.0f);
    }
    else if (m_jitterMode == JitterMode::Hammersley8x)
    {
        uint64_t idx = p_frameCount % 8;
        m_jitterOffset = Hammersley2D(idx, 8) * 2.0f - nm::float2(1.0f);
    }
    else if (m_jitterMode == JitterMode::Hammersley16x)
    {
        uint64_t idx = p_frameCount % 16;
        m_jitterOffset = Hammersley2D(idx, 16) * 2.0f - nm::float2(1.0f);
    }

    // Jitter scale can range between 0.0f and 340282300000000000000000000000000000000.0000f
    // I am unsure why the range is this wide
    m_jitterOffset *= m_jitterScale;

    // TAA is applied post upscaling. Hence we are sampling/jittering over display resolution
    // and not render resolution
    const float offsetX = m_jitterOffset.x() * (1.0f / (float)DISPLAY_RESOLUTION_X);
    const float offsetY = m_jitterOffset.y() * (1.0f / (float)DISPLAY_RESOLUTION_Y);

    nm::Transform jitterMat;
    jitterMat.SetTranslate(nm::float4(offsetX, -offsetY, 0.0f, 1.0f));
    return jitterMat.GetTransform();
}

inline nm::float2 CTAAComputePass::CJitterHelper::Hammersley2D(uint64_t p_sampleIdx, uint64_t p_numSamples)
{
    return nm::float2(float(p_sampleIdx) / float(p_numSamples), RadicalInverseBase2(uint32_t(p_sampleIdx)));
}

inline float CTAAComputePass::CJitterHelper::RadicalInverseBase2(uint32_t p_bits)
{
    p_bits = (p_bits << 16u) | (p_bits >> 16u);
    p_bits = ((p_bits & 0x55555555u) << 1u) | ((p_bits & 0xAAAAAAAAu) >> 1u);
    p_bits = ((p_bits & 0x33333333u) << 2u) | ((p_bits & 0xCCCCCCCCu) >> 2u);
    p_bits = ((p_bits & 0x0F0F0F0Fu) << 4u) | ((p_bits & 0xF0F0F0F0u) >> 4u);
    p_bits = ((p_bits & 0x00FF00FFu) << 8u) | ((p_bits & 0xFF00FF00u) >> 8u);
    return float(p_bits) * 2.3283064365386963e-10f; // / 0x100000000
}