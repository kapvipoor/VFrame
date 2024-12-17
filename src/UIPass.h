#pragma once

#include "Pass.h"

class CUIPass : public CStaticRenderPass
{
public:
	CUIPass(CVulkanRHI*);
	~CUIPass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CDebugDrawPass : public CStaticRenderPass
{
public:
	CDebugDrawPass(CVulkanRHI*);
	~CDebugDrawPass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};