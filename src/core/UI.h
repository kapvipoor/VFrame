#ifndef UI_H
#define UI_H

#include "VulkanRHI.h"

#include <string>
#include <vector>

#include <external/imgui/imgui.h>
#include <external/imgui-filebrowser/imfilebrowser.h>

class CUIParticipant
{
public:
	enum ParticipationType
	{
		pt_none = 0
		, pt_everyFrame = 1		// Update loop is called every frame 
		, pt_onSelect = 2		// Update loop is called every-time on click
	};

	// 
	enum UIDPanelType
	{
		uipt_same = 0		// Adds ui-content to the same panel
		, uipt_new = 1		// Adds ui-content to a new panel
	};

	CUIParticipant(ParticipationType pPartType, UIDPanelType pPanelType, std::string pPanelName = " ");
	~CUIParticipant();

	virtual void Show(CVulkanRHI* p_rhi) = 0;

	ParticipationType GetParticipationType() { return m_participationType; }
	UIDPanelType GetPanelType() { return m_uIDPanelType; }
	std::string GetPanelName() { return m_panelName; }

protected:
	bool m_updated;
	ParticipationType m_participationType;
	UIDPanelType m_uIDPanelType;
	std::string m_panelName;

	ImGui::FileBrowser m_fileDialog;

	bool Header(const char* caption);
	bool CheckBox(const char* caption, bool* value);
	bool CheckBox(const char* caption, int32_t* value);
	bool RadioButton(const char* caption, bool value);
	bool SliderFloat(const char* caption, float* value, float min, float max);
	bool SliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
	bool ComboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
	bool Button(const char* caption);
	void Text(const char* formatstr, ...);
};

class CUIParticipantManager
{
	friend class CUIParticipant;
public:
	CUIParticipantManager();
	~CUIParticipantManager();

	void Show(CVulkanRHI* p_rhi);

private:
	static std::vector<CUIParticipant*> m_uiParticipants;

	void BeginPanel(std::string p_panelName);
	void EndPanel();
};

#endif