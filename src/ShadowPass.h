#pragma once

#include "Pass.h"

class CStaticShadowPrepass : public CStaticRenderPass
{
public:
	CStaticShadowPrepass(CVulkanRHI*);
	~CStaticShadowPrepass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

	bool ReuseShadowMap() { return m_bReuseShadowMap; }

private:
	bool m_bReuseShadowMap;
};