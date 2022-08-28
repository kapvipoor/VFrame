#pragma once

#include "Pass.h"

class CStaticShadowPrepass : public CPass
{
public:
	CStaticShadowPrepass(CVulkanRHI*);
	~CStaticShadowPrepass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};