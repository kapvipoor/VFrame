#pragma once

#include "Pass.h"

class CForwardPass : public CDynamicRenderingPass
{
public:
	CForwardPass(CVulkanRHI*);
	~CForwardPass();

	virtual bool CreateRenderingInfo(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
private:

};

class CSkyboxPass : public CStaticRenderPass
{
public:
	CSkyboxPass(CVulkanRHI*);
	~CSkyboxPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CSkyboxDeferredPass : public CStaticRenderPass
{
public:
	CSkyboxDeferredPass(CVulkanRHI*);
	~CSkyboxDeferredPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CDeferredPass : public CDynamicRenderingPass, CUIParticipant
{
public:
	CDeferredPass(CVulkanRHI*);
	~CDeferredPass();

	virtual bool CreateRenderingInfo(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Show(CVulkanRHI* p_rhi) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

private:
	bool m_enableIBL;
	float m_ambientFactor;
};

class CDeferredLightingPass : public CComputePass
{
public:
	CDeferredLightingPass(CVulkanRHI*);
	~CDeferredLightingPass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Dispatch(RenderData*) override;
private:

};