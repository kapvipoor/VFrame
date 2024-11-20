#pragma once

#include "PostProcessingPasses.h"

CTAA::CTAA()
    : CUIParticipant(CUIParticipant::ParticipationType::pt_everyFrame, CUIParticipant::UIDPanelType::uipt_same)
{
	m_jitterHelper = new CJitterHelper();
}

CTAA::~CTAA()
{
	delete m_jitterHelper;
}

void CTAA::Show(CVulkanRHI* p_rhi)
{
    if (ImGui::TreeNode("TAA"))
    {
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

CTAA::CJitterHelper::CJitterHelper()
    : m_jitterMode(JitterMode::Hammersley4x)
    , m_jitterScale(3.0)
{
}

nm::float4x4 CTAA::CJitterHelper::ComputeJitter(const uint64_t p_frameCount)
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
    const float offsetX = jitter.x()* (1.0f / (float)DISPLAY_RESOLUTION_X);
    const float offsetY = jitter.y()* (1.0f / (float)DISPLAY_RESOLUTION_Y);
    
    nm::Transform jitterOffset;
    jitterOffset.SetTranslate(nm::float4(offsetX, -offsetY, 0.0f, 1.0f));
    return jitterOffset.GetTransform();
}

inline nm::float2 CTAA::CJitterHelper::Hammersley2D(uint64_t p_sampleIdx, uint64_t p_numSamples)
{
    return nm::float2(float(p_sampleIdx) / float(p_numSamples), RadicalInverseBase2(uint32_t(p_sampleIdx)));
}

inline float CTAA::CJitterHelper::RadicalInverseBase2(uint32_t p_bits)
{
    p_bits = (p_bits << 16u) | (p_bits >> 16u);
    p_bits = ((p_bits & 0x55555555u) << 1u) | ((p_bits & 0xAAAAAAAAu) >> 1u);
    p_bits = ((p_bits & 0x33333333u) << 2u) | ((p_bits & 0xCCCCCCCCu) >> 2u);
    p_bits = ((p_bits & 0x0F0F0F0Fu) << 4u) | ((p_bits & 0xF0F0F0F0u) >> 4u);
    p_bits = ((p_bits & 0x00FF00FFu) << 8u) | ((p_bits & 0xFF00FF00u) >> 8u);
    return float(p_bits) * 2.3283064365386963e-10f; // / 0x100000000
}
