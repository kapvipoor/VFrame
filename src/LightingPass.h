#pragma once

#include "Pass.h"

class CForwardPass : public CPass
{
public:
	CForwardPass(CVulkanRHI*);
	~CForwardPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CSkyboxPass : public CPass
{
public:
	CSkyboxPass(CVulkanRHI*);
	~CSkyboxPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CSkyboxDeferredPass : public CPass
{
public:
	CSkyboxDeferredPass(CVulkanRHI*);
	~CSkyboxDeferredPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CDeferredPass : public CPass
{
public:
	CDeferredPass(CVulkanRHI*);
	~CDeferredPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};

class CDeferredLightingPass : public CPass
{
public:
	CDeferredLightingPass(CVulkanRHI*);
	~CDeferredLightingPass();

	virtual bool CreateRenderpass(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;
	virtual void Destroy() override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
};