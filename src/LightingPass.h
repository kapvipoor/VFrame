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
	enum AttachId
	{
		Posiiton = 0
		, Normal_MeshId
		, Color
		, RoughMetal
		, Motion
		, max
	};
};

class CSkyboxPass : public CDynamicRenderingPass
{
public:
	CSkyboxPass(CVulkanRHI*);
	~CSkyboxPass();

	virtual bool CreateRenderingInfo(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
private:
	enum AttachId
	{
		  Color = 0
		, max
	};
};

class CSkyboxDeferredPass : public CDynamicRenderingPass
{
public:
	CSkyboxDeferredPass(CVulkanRHI*);
	~CSkyboxDeferredPass();

	virtual bool CreateRenderingInfo(RenderData* p_renderData) override;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

	virtual bool Update(UpdateData*) override;
	virtual bool Render(RenderData*) override;

	virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;
private:
	enum AttachId
	{
		Color = 0
		, max
	};
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

	float GetAmbientFactor() { return m_ambientFactor; }
	void SetAmbientFactor(float p_factor) { m_ambientFactor = p_factor; }

private:
	enum AttachId
	{
		  Posiiton		= 0
		, Normal_MeshId	= 1
		, Albedo		= 2
		, RoughMetal	= 3
		, Motion		= 4
		, max			= 5
	};

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

class CShadowPass : public CUIParticipant
{
public:
	class CStaticShadowPrepass : public CStaticRenderPass
	{
		friend CShadowPass;
	public:
		CStaticShadowPrepass(CVulkanRHI*);
		~CStaticShadowPrepass();

		virtual bool CreateRenderpass(RenderData*) override;
		virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

		virtual bool Update(UpdateData*) override;
		virtual bool Render(RenderData*) override;

		virtual void GetVertexBindingInUse(CVulkanCore::VertexBinding&)override;

		bool ReuseShadowMap() { return m_bReuseShadowMap; }
		bool IsRTShadowEnabled() { return m_enableRayTracedShadow; }
		void EnableRTShadow(bool p_enable) { m_enableRayTracedShadow = p_enable; }

	private:
		bool m_enableRayTracedShadow;
		bool m_enablePCF;
		bool m_bReuseShadowMap;
	};

	class CRayTraceShadowPass : public CComputePass
	{
		friend CShadowPass;
	public:
		CRayTraceShadowPass(CVulkanRHI*);
		~CRayTraceShadowPass();

		virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

		virtual bool Update(UpdateData*) override;
		virtual bool Dispatch(RenderData*) override;
		
	private:
		float m_minAccumWeight;
	};

	class CShadowDenoisePass : public CComputePass
	{
		friend CShadowPass;
	public:
		CShadowDenoisePass(CVulkanRHI*);
		~CShadowDenoisePass();

		virtual bool CreatePipeline(CVulkanRHI::Pipeline) override;

		virtual bool Update(UpdateData*) override;
		virtual bool Dispatch(RenderData*) override;

	private:
	};

	CShadowPass(CVulkanRHI*);
	~CShadowPass();

	virtual void Show(CVulkanRHI* p_rhi) override;

	CStaticShadowPrepass* GetStaticShadowPrePass() { return m_staticShadowPass; }
	CRayTraceShadowPass* GetRayTraceShadowPass() { return m_rayTraceShadowpass; }
	CShadowDenoisePass* GetShadowDenoisePass() { return m_shadowDenoisePass; }

private:
	CStaticShadowPrepass* m_staticShadowPass;
	CRayTraceShadowPass* m_rayTraceShadowpass;
	CShadowDenoisePass* m_shadowDenoisePass;
};