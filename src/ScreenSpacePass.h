#pragma once

#include "Pass.h"

class CSSRBlurPass : public CComputePass
{
public:
	CSSRBlurPass(CVulkanRHI*);
	~CSSRBlurPass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;
};

class CSSAOComputePass : public CComputePass, CUIParticipant
{
public:
	CSSAOComputePass(CVulkanRHI*);
	~CSSAOComputePass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;

	virtual void Show(CVulkanRHI* p_rhi) override;

private:
	float m_bias;
	nm::float2 m_noiseScale;
	float m_kernelSize;
	float m_kernelRadius;
};

class CSSAOBlurPass : public CComputePass
{
public:
	CSSAOBlurPass(CVulkanRHI*);
	~CSSAOBlurPass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;
};

class CSSRComputePass : public CComputePass, CUIParticipant
{
public:
	CSSRComputePass(CVulkanRHI*);
	~CSSRComputePass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;

	virtual void Show(CVulkanRHI* p_rhi) override;

private:
	float	m_maxDistance;	// Maximum Ray Distance
	float	m_resolution;	// Sampling resolution for the color buffer
	float	m_thickness;	// Ray thickness
	float	m_steps;		// Number of steps for hit point refinement in step 2 (binary search iterations)
};

class CCopyComputePass : public CComputePass
{
public:
	CCopyComputePass(CVulkanRHI*);
	~CCopyComputePass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;

private:
};