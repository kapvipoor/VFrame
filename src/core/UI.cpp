#include "UI.h"



CUIParticipant::CUIParticipant(ParticipationType pPartType, UIDPanelType pPanelType, std::string pPanelName)
	: m_participationType(pPartType)
	, m_uIDPanelType(pPanelType)
	, m_panelName(pPanelName)
{
	// At the moment we do not support creation of new panels if update is
	// called on selection for this participant.
	m_uIDPanelType =
		(m_participationType == ParticipationType::pt_onSelect) ?
		UIDPanelType::uipt_same : m_uIDPanelType;

	CUIParticipantManager::m_uiParticipants.push_back(this);
};

CUIParticipant::~CUIParticipant()
{
	for (int i = 0; i < CUIParticipantManager::m_uiParticipants.size(); i++)
	{
		if (CUIParticipantManager::m_uiParticipants[i] == this)
		{
			CUIParticipantManager::m_uiParticipants.erase(CUIParticipantManager::m_uiParticipants.begin() + i);
			return;
		}
	}
}

bool CUIParticipant::Header(const char* caption)
{
	return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}

bool CUIParticipant::CheckBox(const char* caption, bool* value)
{
	bool res = ImGui::Checkbox(caption, value);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::CheckBox(const char* caption, int32_t* value)
{
	bool val = (*value == 1);
	bool res = ImGui::Checkbox(caption, &val);
	*value = val;
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::RadioButton(const char* caption, bool value)
{
	bool res = ImGui::RadioButton(caption, value);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::SliderFloat(const char* caption, float* value, float min, float max)
{
	bool res = ImGui::SliderFloat(caption, value, min, max);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::SliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
{
	bool res = ImGui::SliderInt(caption, value, min, max);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::ComboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
{
	if (items.empty()) {
		return false;
	}
	std::vector<const char*> charitems;
	charitems.reserve(items.size());
	for (size_t i = 0; i < items.size(); i++) {
		charitems.push_back(items[i].c_str());
	}
	uint32_t itemCount = static_cast<uint32_t>(charitems.size());
	bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
	if (res) { m_updated = true; };
	return res;
}

bool CUIParticipant::Button(const char* caption)
{
	bool res = ImGui::Button(caption);
	if (res) { m_updated = true; };
	return res;
}

void CUIParticipant::Text(const char* formatstr, ...)
{
	va_list args;
	va_start(args, formatstr);
	ImGui::TextV(formatstr, args);
	va_end(args);
}

CUIParticipantManager::CUIParticipantManager()
{

}

CUIParticipantManager::~CUIParticipantManager()
{
	m_uiParticipants.clear();
}

void CUIParticipantManager::Show(CVulkanRHI* p_rhi)
{
	BeginPanel(m_uiParticipants[0]->GetPanelName());
	for (auto& participant : m_uiParticipants)
	{
		if (participant->GetParticipationType() == CUIParticipant::ParticipationType::pt_everyFrame)
		{
			if (participant->GetPanelType() == CUIParticipant::UIDPanelType::uipt_new)
			{
				EndPanel();
				BeginPanel(participant->GetPanelName());
			}
			participant->Show(p_rhi);
			ImGui::Separator();
		}
	}
	EndPanel();
}

void CUIParticipantManager::BeginPanel(std::string p_panelName)
{
	ImGui::SetNextWindowBgAlpha(0.25f); // Transparent background
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::Begin(p_panelName.c_str(), nullptr);// , ImGuiWindowFlags_NoMove);
}

void CUIParticipantManager::EndPanel()
{
	// close current panel
	ImGui::End();
	ImGui::PopStyleVar();
}