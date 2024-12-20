#pragma once

#include "Pass.h"

class CStaticShadowPrepass : public CStaticRenderPass, CUIParticipant
{
public:
	CStaticShadowPrepass(CVulkanRHI*);
	~CStaticShadowPrepass();

	virtual bool CreateRenderpass(RenderData*) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void Show(CVulkanRHI* p_rhi) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

	bool ReuseShadowMap() { return m_bReuseShadowMap; }

private:
	bool m_enablePCF;
	bool m_bReuseShadowMap;
};