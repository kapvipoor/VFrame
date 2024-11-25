#pragma once

#include "PostProcessingPasses.h"


CTAAComputePass::CJitterHelper::CJitterHelper()
    : m_jitterMode(JitterMode::Hammersley4x)
    , m_jitterScale(3.0)
{
}

nm::float4x4 CTAAComputePass::CJitterHelper::ComputeJitter(const uint64_t p_frameCount)
{
    nm::float2 jitter;
    if (m_jitterMode == JitterMode::None)
    {
        return nm::float4x4::identity();
    }
    else if (m_jitterMode == JitterMode::Uniform2x)
    {
        float offset = p_frameCount % 2 == 0 ? -0.5f : 0.5f;
        jitter = nm::float2(offset);
    }
    else if (m_jitterMode == JitterMode::Hammersley4x)
    {
        uint64_t idx = p_frameCount % 4;
        jitter = Hammersley2D(idx, 4) * 2.0f - nm::float2(1.0f);
    }
    else if (m_jitterMode == JitterMode::Hammersley8x)
    {
        uint64_t idx = p_frameCount % 8;
        jitter = Hammersley2D(idx, 8) * 2.0f - nm::float2(1.0f);
    }
    else if (m_jitterMode == JitterMode::Hammersley16x)
    {
        uint64_t idx = p_frameCount % 16;
        jitter = Hammersley2D(idx, 16) * 2.0f - nm::float2(1.0f);
    }

    // Jitter scale can range between 0.0f and 340282300000000000000000000000000000000.0000f
    // I am unsure why the range is this wide
    jitter *= m_jitterScale;

    // TAA is applied post upscaling. Hence we are sampling/jittering over display resolution
    // and not render resolution
    const float offsetX = jitter.x() * (1.0f / (float)DISPLAY_RESOLUTION_X);
    const float offsetY = jitter.y() * (1.0f / (float)DISPLAY_RESOLUTION_Y);

    nm::Transform jitterOffset;
    jitterOffset.SetTranslate(nm::float4(offsetX, -offsetY, 0.0f, 1.0f));
    return jitterOffset.GetTransform();
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


CTAAComputePass::CTAAComputePass(CVulkanRHI* p_rhi)
    : CPass(p_rhi)
    , CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
{
    m_frameBuffer.resize(1);
    m_isEnabled = false; // for now disabling it by default
	m_jitterHelper = new CJitterHelper();
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
    //uint32_t scId = p_renderData->scIdx;
    //CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
    //const CPrimaryDescriptors* primaryDesc = p_renderData->primaryDescriptors;
    //uint32_t dispatchDim_x = m_rhi->GetRenderWidth() / THREAD_GROUP_SIZE_X;
    //uint32_t dispatchDim_y = m_rhi->GetRenderHeight() / THREAD_GROUP_SIZE_Y;
    //
    //if (!m_rhi->BeginCommandBuffer(cmdBfr, "Compute TAA"))
    //    return false;
    //
    //p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSAO);
    //p_renderData->fixedAssets->GetRenderTargets()->IssueLayoutBarrier(m_rhi, VK_IMAGE_LAYOUT_GENERAL, cmdBfr, CRenderTargets::rt_SSAOBlur);
    //
    //vkCmdBindPipeline(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeline);
    //vkCmdBindDescriptorSets(cmdBfr, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.pipeLayout, BindingSet::bs_Primary, 1, primaryDesc->GetDescriptorSet(scId), 0, nullptr);
    //vkCmdDispatch(cmdBfr, dispatchDim_x, dispatchDim_y, 1);
    //
    //m_rhi->EndCommandBuffer(cmdBfr);

    CVulkanRHI::CommandBuffer cmdBfr = p_renderData->cmdBfr;
    CVulkanRHI::Image colorRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_PrimaryColor);
    CVulkanRHI::Image prevColorlRT = p_renderData->fixedAssets->GetRenderTargets()->GetTexture(CRenderTargets::rt_Prev_PrimaryColor);

    RETURN_FALSE_IF_FALSE(m_rhi->BeginCommandBuffer(cmdBfr, "TAA Compute Pass"));
    {
        m_rhi->InsertMarker(cmdBfr, "Temporal Color Copy");
        m_rhi->CopyImage(cmdBfr, colorRT, prevColorlRT);
    }
    RETURN_FALSE_IF_FALSE(m_rhi->EndCommandBuffer(cmdBfr));

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
    bool isEnabled = IsEnabled();
    ImGui::Checkbox(" ", &isEnabled);
    Enable(isEnabled);

    if (m_isEnabled == false)
    {
        m_jitterHelper->m_jitterMode = CJitterHelper::JitterMode::None;
    }

    ImGui::SameLine(25);

    if (ImGui::TreeNode("TAA"))
    {
        ImGui::Indent();
        if (ImGui::TreeNode("Jitter"))
        {
            ImGui::SliderFloat("Jitter Scale", &m_jitterHelper->m_jitterScale, 0.00f, 50.0f);

            const char* items[] = { "None", "Uniform2x", "Hammersley4x", "Hammersley8x", "Hammersley16x" };
            int jitMod = (int)m_jitterHelper->m_jitterMode;
            ImGui::ListBox("Jitter Mode", &jitMod, items, IM_ARRAYSIZE(items), 4);
            m_jitterHelper->m_jitterMode = (CJitterHelper::JitterMode)jitMod;

            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
}