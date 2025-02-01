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

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&) override;
};

class CDebugDrawPass : public CDynamicRenderingPass
{
public:
	CDebugDrawPass(CVulkanRHI*);
	~CDebugDrawPass();

	virtual bool CreateRenderingInfo(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&) override;

private:
	enum AttachId
	{
		Color = 0
		, max
	};
};