#pragma once

#include "Pass.h"

class CSSAOComputePass : public CPass
{
public:
	CSSAOComputePass(CVulkanRHI*);
	~CSSAOComputePass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

private:
};

class CSSAOBlurPass : public CPass
{
public:
	CSSAOBlurPass(CVulkanRHI*);
	~CSSAOBlurPass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CSSRComputePass : public CPass, CUIParticipant
{
public:
	CSSRComputePass(CVulkanRHI*);
	~CSSRComputePass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

	virtual void Show(CVulkanRHI* p_rhi) override;

	float GetMaxDistance() { return m_maxDistance; }
	float GetResolution() { return m_resolution; }
	float GetThickness() { return m_thickness; }
	float GetSteps() { return m_steps; }

private:
	float	m_maxDistance;	// Maximum Ray Distance
	float	m_resolution;	// Sampling resolution for the color buffer
	float	m_thickness;	// Ray thickness
	float	m_steps;		// Number of steps for hit point refinement in step 2 (binary search iterations)
};

class CCopyComputePass : public CPass
{
public:
	CCopyComputePass(CVulkanRHI*);
	~CCopyComputePass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

private:
};