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

class CSSRComputePass : public CPass
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

private:
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