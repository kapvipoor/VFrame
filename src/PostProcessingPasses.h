#pragma once

#include "Pass.h"
#include "core/UI.h"

class CTAAComputePass : public CPass, CUIParticipant
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

		CJitterHelper();
		~CJitterHelper() {}

		nm::float4x4 ComputeJitter(const uint64_t p_frameCount);

	private:
		JitterMode m_jitterMode;
		float m_jitterScale;

		// Returns a single 2D point in a Hammersley sequence of length "numSamples", using base 1 and base 2
		// Refer - https://mathworld.wolfram.com/HammersleyPointSet.html
		inline nm::float2 Hammersley2D(uint64_t p_sampleIdx, uint64_t p_numSamples);

		// Computes a radical inverse with base 2 using crazy bit-twiddling from "Hacker's Delight"
		// Refer - https://graphics.stanford.edu/~seander/bithacks.html
		inline float RadicalInverseBase2(uint32_t p_bits);
	};

	CTAAComputePass(CVulkanRHI* p_rhi);
	~CTAAComputePass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

	virtual void Show(CVulkanRHI* p_rhi) override;

	CJitterHelper* GetJitterHelper() { return m_jitterHelper; }

private:
	CJitterHelper* m_jitterHelper;
};