#ifndef PASS_H
#define PASS_H

#include "core/VulkanRHI.h"
#include "core/Asset.h"

extern uint32_t g_passIndex;

class CPass
{
public:
	struct RenderData
	{
		uint32_t					scIdx;
		CVulkanRHI::CommandBuffer	cmdBfr;
		CLoadableAssets*			loadedAssets;
		CFixedAssets*				fixedAssets;
		const CPrimaryDescriptors*	primaryDescriptors;
		CVulkanRHI::BufferList*		stagingBuffers;			// used during the initializing phase
		CSceneGraph*				sceneGraph;
		CVulkanRHI::RendererType	rendererType;
	};

	struct UpdateData
	{
		float						timeElapsed;
		nm::float2					curMousePos;
		nm::float2					screenRes;
		bool						leftMouseDown;
		bool						rightMouseDown;
		CSceneGraph*				sceneGraph;
		PrimaryUniformData*			uniformData;
	};

	CPass(CVulkanRHI* p_core);
	~CPass();

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) = 0;
	
	virtual bool Update(UpdateData*) = 0;
	virtual void Destroy() {
		m_rhi->DestroyPipeline(m_pipeline);
	}

	void Enable(bool p_enable) { m_isEnabled = p_enable; }
	bool IsEnabled() { return m_isEnabled; }

protected:
	bool m_isEnabled;
	CVulkanRHI* m_rhi;
	CVulkanRHI::Pipeline m_pipeline;

	uint32_t m_passIndex;
private:
};

class CComputePass : public CPass
{
public:
	CComputePass(CVulkanRHI* p_core)
		: CPass(p_core) {}

	~CComputePass() {}

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) = 0;

	virtual bool Initalize(CVulkanRHI::Pipeline p_pipeline)
	{
		RETURN_FALSE_IF_FALSE(CreatePipeline(p_pipeline));
		return true;
	}
	virtual bool Update(UpdateData*) = 0;
	virtual bool Dispatch(RenderData*) = 0;

protected:

};

class CRasterizationPass : public CPass
{
public:
	CRasterizationPass(CVulkanRHI* p_core)
		: CPass(p_core) {}

	~CRasterizationPass() {}

	virtual bool CreatePipeline(CVulkanRHI::Pipeline) = 0;

	virtual bool Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline) = 0;
	virtual bool Update(UpdateData*) = 0;
	virtual bool Render(RenderData*) = 0;
		
	virtual void GetVertexBindingInUse(CVulkanRHI::VertexBinding&) = 0;

protected:

};

class CStaticRenderPass : public CRasterizationPass
{
public:
	CStaticRenderPass(CVulkanRHI* p_core)
		:CRasterizationPass(p_core) {}

	~CStaticRenderPass() {}

	virtual bool CreateRenderpass(RenderData*) = 0;

	virtual bool Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline)
	{
		RETURN_FALSE_IF_FALSE(CreateRenderpass(p_renderData));
		RETURN_FALSE_IF_FALSE(CreatePipeline(p_pipeline));
		return true;
	}

	virtual bool Update(UpdateData*) = 0;
	virtual bool Render(RenderData*) = 0;
	virtual void Destroy() override {
		for (auto& frameBuffer : m_frameBuffer)
		{
			m_rhi->DestroyFramebuffer(frameBuffer);
		}
		m_rhi->DestroyRenderpass(m_pipeline.renderpassData.renderpass);
		CPass::Destroy();
	}

	void SetFrameBuffer(CVulkanRHI::FrameBuffer& p_fb) { m_frameBuffer = p_fb; }

	CVulkanRHI::Renderpass GetRenderpass() { return m_pipeline.renderpassData; }
	CVulkanRHI::FrameBuffer& GetFrameBuffer() { return m_frameBuffer; }

	virtual void GetVertexBindingInUse(CVulkanRHI::VertexBinding&) = 0;

protected:
	CVulkanRHI::FrameBuffer m_frameBuffer;
};

class CDynamicRenderingPass : public CRasterizationPass
{
public:
	CDynamicRenderingPass(CVulkanRHI* p_core)
		:CRasterizationPass(p_core) {}

	~CDynamicRenderingPass() {}

	virtual bool CreateRenderingInfo(RenderData*) = 0;

	virtual bool Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline)
	{
		RETURN_FALSE_IF_FALSE(CreateRenderingInfo(p_renderData));
		RETURN_FALSE_IF_FALSE(CreatePipeline(p_pipeline));
		return true;
	}

	virtual bool Update(UpdateData*) = 0;
	virtual bool Render(RenderData*) = 0;

	virtual void GetVertexBindingInUse(CVulkanRHI::VertexBinding&) = 0;

protected:
	std::vector<VkRenderingAttachmentInfo> m_colorAttachInfos;
	VkRenderingAttachmentInfo m_depthAttachInfo;
	VkRenderingInfo m_renderingInfo;
};

#endif