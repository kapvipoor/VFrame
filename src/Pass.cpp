#include "Pass.h"

uint32_t g_passIndex = 0;;

CPass::CPass(CVulkanRHI* p_rhi)
	: m_rhi(p_rhi)
	, m_isEnabled(true)
{
	m_passIndex = g_passIndex++;
}

CPass::~CPass()
{}