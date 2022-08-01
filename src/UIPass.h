#pragma once

#include "Pass.h"

class CUIPass : public CPass
{
public:
	CUIPass(CVulkanRHI*);
	~CUIPass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Initalize(RenderData*, CVulkanRHI::Pipeline) override;
	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};