#include "SceneGraph.h"
#include "external/imgui/imgui.h"
#include "external/imguizmo/ImGuizmo.h"

// These entities do not tribute to the
// bounds of the scene hence when recalculating the bounds,
// we choose to not include them
enum SceneBoundsNonContributors
{
	 sunlight = 0 // primary directional light
	,skybox
	,count
};

std::vector<CEntity*> CSceneGraph::s_entities;
bool CSceneGraph::s_bShouldRecomputeSceneBBox = false;
std::vector<CSelectionListener*> CSelectionBroadcast::m_listeneers;
nm::Transform CSceneGraph::s_sceneTransform = nm::Transform();

CUIParticipant::CUIParticipant(ParticipationType pType)
	: m_participationType(pType)
{
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

void CUIParticipantManager::Show()
{
	for (auto& participant : m_uiParticipants)
	{
		if (participant->GetParticipationType() == CUIParticipant::ParticipationType::pt_everyFrame)
		{
			participant->Show();
			ImGui::Separator();
		}
	}
}

CSelectionListener::CSelectionListener()
{
	m_selectedSubMeshIndex = -1;
	CSelectionBroadcast::m_listeneers.push_back(this);
};

CSelectionBroadcast::~CSelectionBroadcast()
{
	m_listeneers.clear();
}
void CSelectionBroadcast::Broadcast(CSceneGraph* p_sceneGraph, int p_entityId)
{
	CEntity* selectedEntity = (*p_sceneGraph->GetEntities())[p_entityId];

	// if previously selected entity is same as currently selected entity; return
	if (m_listeneers[0]->GetCurSelectEntity() == selectedEntity)
	{
		return;
	}

	// else broadcast
	for (auto& listener : m_listeneers)
	{
		listener->m_selectedEntity = selectedEntity;
		//listener->OnSelectionChange(selectedEntity);
	}
}

CSceneGraph::CSceneGraph(CPerspectiveCamera* p_primaryCamera)
{
	m_primaryCamera			= p_primaryCamera;
	m_selectionBroadcast	= new CSelectionBroadcast();
}

CSceneGraph::~CSceneGraph()
{
	delete m_selectionBroadcast;
}

bool CSceneGraph::Update()
{
	m_sceneStatus = SceneStatus::ss_NoChange;
	BBox prevBBox = m_boundingBox;
	if (s_bShouldRecomputeSceneBBox == true)
	{
		m_boundingBox.Reset();
		s_bShouldRecomputeSceneBBox = false;
		
		for (int i = SceneBoundsNonContributors::count; i < s_entities.size(); i++)
		{
			m_boundingBox.Merge((*s_entities[i]->GetBoundingBox()) * s_entities[i]->GetTransform());
			m_sceneStatus = SceneStatus::ss_SceneMoved;
		}
	}
	if (!(prevBBox == m_boundingBox))
	{
		m_sceneStatus = SceneStatus::ss_BoundsChange;
	}

	return true;
}

void CSceneGraph::SetCurSelectEntityId(int p_entityId)
{
	if (p_entityId < -1 || p_entityId > GetEntities()->size())
	{
		return;
	}

	m_selectionBroadcast->Broadcast(this, p_entityId);
}

uint32_t CSceneGraph::RegisterEntity(CEntity* p_entity)
{
	uint32_t count = (uint32_t)s_entities.size();
	s_entities.push_back(p_entity);
	return count;
}

void CSceneGraph::RequestSceneBBoxUpdate()
{
	s_bShouldRecomputeSceneBBox = true;
}

CEntity::CEntity(std::string p_name)
	: CUIParticipant(CUIParticipant::ParticipationType::pt_onSelect)
{
	m_dirty						= false;
	m_id						= CSceneGraph::RegisterEntity(this);
	m_name						= p_name + "_" + std::to_string(m_id);
}

void CEntity::Show()
{
	ImGui::Indent();

	nm::float4x4 transform = GetTransform().GetTransform();
	float matrixTranslation[3], matrixRotation[3], matrixScale[3];
	ImGuizmo::DecomposeMatrixToComponents(&transform.column[0][0], matrixTranslation, matrixRotation, matrixScale);
	{
		ImGui::Text("Translation ");
		ImGui::PushItemWidth(45);
		ImGui::SameLine(150); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.5f, 0.0f, 0.0f)); ImGui::InputFloat("X", &matrixTranslation[0]);
		ImGui::SameLine(225); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.0f, 0.5f, 0.0f)); ImGui::InputFloat("Y", &matrixTranslation[1]);
		ImGui::SameLine(300); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.0f, 0.0f, 0.5f)); ImGui::InputFloat("Z", &matrixTranslation[2]);
		ImGui::PopStyleColor(3);
		ImGui::PopItemWidth();
	}

	{
		ImGui::Text("Rotation ");
		ImGui::PushItemWidth(45);
		ImGui::SameLine(150); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.5f, 0.0f, 0.0f)); ImGui::InputFloat("X", &matrixRotation[0]);
		ImGui::SameLine(225); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.0f, 0.5f, 0.0f)); ImGui::InputFloat("Y", &matrixRotation[1]);
		ImGui::SameLine(300); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.0f, 0.0f, 0.5f)); ImGui::InputFloat("Z", &matrixRotation[2]);
		ImGui::PopStyleColor(3);
		ImGui::PopItemWidth();
	}

	{
		ImGui::Text("Scale ");
		ImGui::PushItemWidth(45);
		ImGui::SameLine(150); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.5f, 0.0f, 0.0f)); ImGui::InputFloat("X", &matrixScale[0]);
		ImGui::SameLine(225); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.0f, 0.5f, 0.0f)); ImGui::InputFloat("Y", &matrixScale[1]);
		ImGui::SameLine(300); ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor(0.0f, 0.0f, 0.5f)); ImGui::InputFloat("Z", &matrixScale[2]);
		ImGui::PopStyleColor(3);
		ImGui::PopItemWidth();
	}

	ImGui::Unindent();
}

void CEntity::SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox)
{
	m_dirty						= true;
	m_transform					= p_transform;

	if(p_bRecomputeSceneBBox)
		CSceneGraph::RequestSceneBBoxUpdate();
}

nm::Transform& CEntity::GetTransform()
{
	return m_transform;
}

void CEntity::SetBoundingBox(BBox p_bbox, bool p_bRecomputeSceneBBox)
{
	m_boundingBox				= p_bbox;

	if(p_bRecomputeSceneBBox)
		CSceneGraph::RequestSceneBBoxUpdate();
}

CDebugData::CDebugData()
{
	m_debugType					= DebugType::Box;
	m_debugDrawSubmeshes		= false;
	m_debugDraw					= false;
}

CLight::CLight(std::string p_name, Type p_type, bool p_castShadow)
	:CEntity(p_name)
	, m_type(p_type)
	, m_castShadow(p_castShadow)
	, m_intensity(1.0f)
	, m_color(nm::float3(1.0f))
{
}

void CLight::Show()
{
	CEntity::Show();

	ImGui::Indent();
	ImGui::Text("Light Type: %s", (m_type == Type::Directional ? "Directional" : "Point"));
	ImGui::Text("Cast Shadow: %s", (m_castShadow == true? "true" : "false"));
	ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 20.0f);
	ImGui::InputFloat3("Color ", &m_color[0]);
	ImGui::Unindent();
}

//CPointLight::CPointLight(std::string p_name)
//	: CLight(p_name)
//{
//	m_drawAsSphere = true;
//}
//
//void CPointLight::SetTransform(nm::Transform p_transform)
//{
//	m_dirty			= true;
//	
//	// for point lights; we want to
//	// 1. neutralize rotation matrix
//	p_transform.SetRotation(nm::float4x4::identity());
//
//	// 2. Apply uniform scaling matrix so that scaling happens in all axis uniformly
//	nm::float3 scale = p_transform.GetScaleVector();
//	float scaleFactor = (scale[0] == scale[1]) ? scale[2] : ((scale[1] == scale[2]) ? scale[0] : ((scale[0] == scale[2]) ? scale[1] : 1.0f));
//	p_transform.SetScale(nm::float4x4::identity() * scaleFactor);
//
//	m_transform		= p_transform;
//	CSceneGraph::RequestSceneBBoxUpdate();
//}

CDirectionaLight::CDirectionaLight(std::string p_name, bool p_castShadow, nm::float3 p_direction)
	:CLight(p_name, Type::Directional, p_castShadow)
	, m_direction(p_direction)
{
	m_camera = new COrthoCamera();
}

CDirectionaLight::CDirectionaLight(std::string p_name, bool p_castShadow, nm::float3 p_direction, float intensity, nm::float3 color)
	:CLight(p_name, Type::Directional, p_castShadow)
	, m_direction(p_direction)
{
	m_intensity = intensity;
	m_color = color;
	m_camera = new COrthoCamera();
}

CDirectionaLight::~CDirectionaLight()
{
	delete m_camera;
}

bool CDirectionaLight::Init(const CCamera::InitData& p_data)
{
	// Re-initialize the shadow camera's view and projection matrix because we want the 
	// shadow camera to properly fit the new scene bounds
	RETURN_FALSE_IF_FALSE(m_camera->Init(const_cast<COrthoCamera::InitData*>(&p_data)));

	/// Recompute the bounding box of the shadow camera for debug visibility
	// the z values in Vulkan ranges between 0-1 and not -1 and 1
	// so, min.z = 0 and max.z = 1
	BBox box = BBox(BBox::Custom, BBox::CameraStyle, nm::float3(-1.0f, -1.0f, 0.0f), nm::float3(1.0f, 1.0f, 1.0f));

	// Convert this bounding box from View space to world space by multiplying with inverse view projection matrix
	box = box * nm::inverse(m_camera->GetViewProj());

	SetBoundingBox(box, false /*do not recalculate the bounding box of the scene graph*/);

	//m_transform.SetRotation(0.0f, -m_camera->GetPitch(), m_camera->GetYaw());
	//m_transform.SetTranslate(nm::float4(m_camera->GetLookFrom(), 1.0f));

	return true;
}

bool CDirectionaLight::Update(const CCamera::UpdateData& p_data)
{
	if (CEntity::m_dirty)
	{
		m_direction = (m_transform.GetRotate() * nm::float4(m_direction, 1.0f)).xyz();

		//CCamera::UpdateData camUpdateData = p_data;
		//camUpdateData.transform = m_transform;
		//m_camera->Update(camUpdateData);

		CEntity::m_dirty = false;
	}

	return true;
}

void CDirectionaLight::SetTransform(nm::Transform p_transform)
{
	// we do not expect directional light to trigger
	// re-computation of scene bounding box
	CEntity::SetTransform(p_transform, false);
}

void CDirectionaLight::Show()
{
	CLight::Show();

	ImGui::Indent();
	ImGui::InputFloat3("Direction ", &m_direction[0]);
	ImGui::Unindent();
}

//void CDirectionaLight::Show()
//{
//	CEntity::Show();
//}

/*
Initialization
1. Construct the Directional light with its shadow camera the way we want it (looking downwards)

Update - Scene change
1. The Light's entity transform remains unchanged
2. If the scene bounds change, we want the shadow camera to recompute its bounding box.

Update - Gizmo interaction with Directional Light
1. Only rotation is valid, do not allow translation or scaling
2. When rotation occurs, update the transform of the entity (the bounding box of the entity will apply as is)
3. Apply transform to shadow camera

*/

CPointLight::CPointLight(std::string p_name, bool p_castShadow, nm::float3 p_position, float intensity, nm::float3 color)
	: CLight(p_name, Type::Point, p_castShadow)
	, m_position(p_position)
{
	CEntity::m_transform.SetTranslate(nm::float4(p_position, 1.0f));
	CEntity::m_transform.SetScale(nm::float3(intensity));
	m_intensity			= intensity;
	m_color				= color;
	m_debugType			= CDebugData::DebugType::Sphere; 
}

bool CPointLight::Init(const CCamera::InitData&)
{
	BBox box = BBox(BBox::Unit, BBox::Center);
	SetBoundingBox(box, true /*we expect point light to trigger re-computation of scene */);
	return true;
}

bool CPointLight::Update(const CCamera::UpdateData&)
{
	if (CEntity::m_dirty)
	{
		m_position = m_transform.GetTranslateVector();
		m_intensity = m_transform.GetScaleVector()[0];  // assuming uniform scaling on all axis

		CEntity::m_dirty = false;
	}

	return true;
}

void CPointLight::SetTransform(nm::Transform p_transform)
{
	// we expect point light to trigger re-computation of scene 
	// bounding box if it goes out of scene bounds
	CEntity::SetTransform(p_transform);
}

void CPointLight::Show()
{
	CLight::Show();

	ImGui::Indent();
	ImGui::InputFloat3("Position ", &m_position[0]);
	ImGui::Unindent();
}

CLights::CLights()
	: m_isDirty(false)
{
	//CreateLight(CLight::Type::Directional, "Sunlight", false, nm::float3(1.0f, 1.0f, 0.99f), 10.0f, nm::float3(0.0f, 1.0f, 0.0f));
	CreateLight(CLight::Type::Point, "PointLight_A", false, nm::float3(1.0f, 1.0f, 0.0f), 2.0f, nm::float3(0.0f, 0.0f, 0.0f));
	//CreateLight(CLight::Type::Point, "PointLight_B", false, nm::float3(0.0f, 1.0f, 1.0f), 2.0f, nm::float3(1.0f, 0.0f, 0.0f));
	//CreateLight(CLight::Type::Point, "PointLight_C", false, nm::float3(1.0f, 0.0f, 1.0f), 2.0f, nm::float3(0.0f, 0.0f, 1.0f));
}

void CLights::Update(const CCamera::UpdateData& p_updateData)
{
	for (int i = 0; i < m_lights.size(); i++)
	{
		if (m_lights[i]->IsDirty())
		{
			m_lights[i]->Update(p_updateData);

			nm::float3 vector3 = (m_lights[i]->GetType() == CLight::Type::Directional) ?
				static_cast<CDirectionaLight*>(m_lights[i])->GetDirection() :
				static_cast<CPointLight*>(m_lights[i])->GetPosition();

			m_rawGPUData[i].intensity = m_lights[i]->GetIntensity();
			m_rawGPUData[i].vector3[0] = vector3[0];
			m_rawGPUData[i].vector3[1] = vector3[1];
			m_rawGPUData[i].vector3[2] = vector3[2];

			m_isDirty = true;
		}
	}
}

void CLights::CreateLight(CLight::Type p_type, const char* p_name, bool p_castShadow, nm::float3 p_color, float p_intensity, nm::float3 p_vector3)
{
	CLight* light = nullptr;

	switch (p_type)
	{
	case CLight::Type::Directional:
		light = new CDirectionaLight(p_name, p_castShadow, p_vector3 /*position*/, p_intensity, p_color);
		break;

	case CLight::Type::Point:
		light = new CPointLight(p_name, p_castShadow, p_vector3 /*position*/, p_intensity, p_color);
		break;

	default:
		return;
	}

	light->SetId((uint32_t)m_lights.size());

	CPerspectiveCamera::PerpspectiveInitdData data{};
	light->Init(data);

	m_lights.push_back(light);

	LightGPUData gpuData{};
	gpuData.type_castShadow = ((uint32_t)light->GetType() << 16) | (uint32_t)(light->IsCastsShadow());
	gpuData.color[0] = p_color[0];
	gpuData.color[1] = p_color[1];
	gpuData.color[2] = p_color[2];
	gpuData.intensity = p_intensity;
	gpuData.vector3[0] = p_vector3[0];
	gpuData.vector3[1] = p_vector3[1];
	gpuData.vector3[2] = p_vector3[2];
	m_rawGPUData.push_back(gpuData);
}

void CLights::DestroyLights()
{
	for (auto& light : m_lights)
	{
		delete light;
	}

	m_lights.clear();
	m_rawGPUData.clear();
}