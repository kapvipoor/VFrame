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
{
	m_dirty						= false;
	m_id						= CSceneGraph::RegisterEntity(this);
	m_name						= p_name + "_" + std::to_string(m_id);
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
	m_debugDrawSubmeshes		= false;
	m_debugDraw					= false;
	m_drawAsSphere				= false;
}