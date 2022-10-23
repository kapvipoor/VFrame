#ifndef PASS_H
#define PASS_H

#include "core/VulkanRHI.h"
#include "core/Asset.h"

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
	};

	CPass(CVulkanRHI* p_core)
		: m_rhi(p_core)
	{}
	~CPass() {}


	virtual bool CreateRenderpass(RenderData*) = 0;
	virtual bool CreatePipeline(CVulkanRHI::Pipeline) = 0;

	virtual bool Initalize(RenderData* p_renderData, CVulkanRHI::Pipeline p_pipeline)
	{
		RETURN_FALSE_IF_FALSE(CreateRenderpass(p_renderData));
		RETURN_FALSE_IF_FALSE(CreatePipeline(p_pipeline));
		return true;
	}

	virtual bool Update(UpdateData*) = 0;
	virtual bool Render(RenderData*) = 0;
	virtual void Destroy() = 0;

	virtual void GetVertexBindingInUse(CVulkanRHI::VertexBinding&) = 0;

	CVulkanRHI::Renderpass GetRenderpass() { return m_pipeline.renderpassData; }
	CVulkanRHI::FrameBuffer& GetFrameBuffer() { return m_frameBuffer; }

	void SetFrameBuffer(CVulkanRHI::FrameBuffer& p_fb) { m_frameBuffer = p_fb; }

protected:
	CVulkanRHI*						m_rhi;
	CVulkanRHI::FrameBuffer			m_frameBuffer;
	CVulkanRHI::Pipeline			m_pipeline;
};

#endif