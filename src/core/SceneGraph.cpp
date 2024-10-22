#include "SceneGraph.h"
#include "Asset.h"
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

CSelectionListener::CSelectionListener()
{
	CSelectionBroadcast::m_listeneers.push_back(this);
};

CSelectionBroadcast::~CSelectionBroadcast()
{
	m_listeneers.clear();
}
void CSelectionBroadcast::BroadcastEntityId(CSceneGraph* p_sceneGraph, int p_entityId)
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
	}
}

void CSelectionBroadcast::BroadcastSubmeshId(CSceneGraph* p_sceneGraph, int p_submeshId)
{
	// if previously selected entity is same as currently selected entity; return
	if (p_submeshId < 0 || m_listeneers[0]->GetSelectedSubMeshId() == p_submeshId)
	{
		return;
	}

	// else broadcast
	for (auto& listener : m_listeneers)
	{
		listener->m_selectedSubMeshId = p_submeshId;
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

void CSceneGraph::Update()
{
	m_sceneStatus = SceneStatus::ss_NoChange;
	BBox prevBBox = m_boundingBox;

	if (s_bShouldRecomputeSceneBBox == true)
	{
		m_boundingBox.Reset();
		s_bShouldRecomputeSceneBBox = false;
		m_boundingBox = BBox(BBox::Type::Unit, BBox::Origin::Center);

		// only Render-able Meshes are allowed to participate in the the scene
		// bounding box for now. Other entities like Lights and skybox do not
		// participate.
		//for (int i = 0; i < s_entities.size(); i++)
		for (auto& entity : s_entities)
		{
			CRenderableMesh* mesh = dynamic_cast<CRenderableMesh*>(entity);
			if (mesh)
			{
				BBox* meshBox = dynamic_cast<BBox*>(entity->GetBoundingVolume());
				if (meshBox)
				{
					BBox entityBbox = (*meshBox) * entity->GetTransform();
					m_boundingBox.Merge(entityBbox);
					m_sceneStatus = SceneStatus::ss_SceneMoved;
				}
			}
		}
	}
	if (!(prevBBox == m_boundingBox))
	{
		m_sceneStatus = SceneStatus::ss_BoundsChange;
	}
}

void CSceneGraph::SetCurSelectEntityId(int p_entityId)
{
	if (p_entityId < 0 || p_entityId > GetEntities()->size())
	{
		return;
	}

	m_selectionBroadcast->BroadcastEntityId(this, p_entityId);
}

void CSceneGraph::SetCurSelectedSubMeshId(int p_meshId)
{
	if (p_meshId < 0)
	{
		return;
	}

	m_selectionBroadcast->BroadcastSubmeshId(this, p_meshId);
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
	: CUIParticipant(CUIParticipant::ParticipationType::pt_onSelect, CUIParticipant::UIDPanelType::uipt_same)
 {
	m_dirty						= false;
	m_id						= CSceneGraph::RegisterEntity(this);
	m_name						= p_name + "_" + std::to_string(m_id);
}

void CEntity::Show(CVulkanRHI* p_rhi)
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

nm::Transform& CEntity::GetTransform()
{
	return m_transform;
}

void CEntity::SetBoundingVolume(BVolume* p_bvol, bool p_bRecomputeSceneBBox)
{
	m_boundingVolume = p_bvol;

	if (p_bRecomputeSceneBBox)
		CSceneGraph::RequestSceneBBoxUpdate();
}

CDebugData::CDebugData()
{
	m_debugDrawSubmeshes		= false;
	m_debugDraw					= false;
}