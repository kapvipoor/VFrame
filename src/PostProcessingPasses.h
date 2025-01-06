#pragma once

#include "Pass.h"
#include "core/UI.h"

class CToneMapPass : public CStaticRenderPass, CUIParticipant
{
public:
	enum ToneMapper
	{
		  None = 0
		, Reinhard
		, AMD
	};

	CToneMapPass(CVulkanRHI*);
	~CToneMapPass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

	virtual void Show(CVulkanRHI* p_rhi) override;

private:
	ToneMapper m_toneMapper;
	float m_exposure;
};

class CTAAComputePass : public CComputePass, CUIParticipant
{
public:
	class CJitterHelper
	{
		friend CTAAComputePass;
	public:
		enum JitterMode
		{
			None = 0
			, Uniform2x = 1
			, Hammersley4x = 2
			, Hammersley8x = 3
			, Hammersley16x = 4
			, jm_max
		};

		CJitterHelper(JitterMode);
		~CJitterHelper() {}

		nm::float4x4 ComputeJitter(const uint32_t p_frameCount);
		nm::float2 GetJitterOffset() { return m_jitterOffset; }

	private:
		JitterMode m_jitterMode;
		float m_jitterScale;
		nm::float2 m_jitterOffset;

		// Returns a single 2D point in a Hammersley sequence of length "numSamples", using base 1 and base 2
		// Refer - https://mathworld.wolfram.com/HammersleyPointSet.html
		inline nm::float2 Hammersley2D(uint32_t p_sampleIdx, uint32_t p_numSamples);

		// Computes a radical inverse with base 2 using crazy bit-twiddling from "Hacker's Delight"
		// Refer - https://graphics.stanford.edu/~seander/bithacks.html
		inline float RadicalInverseBase2(uint32_t p_bits);
	};

	enum FlickerCorrection
	{
		  None = 0
		, LogWeighing
		, LuminanceWeighing
	};

	enum ReprojectionFilter
	{
		  Standard = 0
		, CatmullRom
	};

	CTAAComputePass(CVulkanRHI* p_rhi);
	~CTAAComputePass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;

	virtual void Show(CVulkanRHI* p_rhi) override;

	CJitterHelper* GetJitterHelper() { return m_jitterHelper; }

private:
	CJitterHelper* m_jitterHelper;
	CJitterHelper::JitterMode m_activeJitterMode;

	float m_resolveWeight;
	bool m_useMotionVectors;
	FlickerCorrection m_flickerCorrectionMode;
	ReprojectionFilter m_reprojectionFilter;

};