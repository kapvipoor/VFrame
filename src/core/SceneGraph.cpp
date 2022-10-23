#include "SceneGraph.h"
#include "external/imgui/imgui.h"
#include "external/imguizmo/ImGuizmo.h"

std::vector<CEntity*> CSceneGraph::s_entities;
std::vector<CSelectionListener*> CSelectionBroadcast::m_listeneers;

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

	// if previously seleted entity is same as currently selected entity; return
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

CSceneGraph::CSceneGraph()
{
	m_selectionBroadcast	= new CSelectionBroadcast();
}

CSceneGraph::~CSceneGraph()
{
	delete m_selectionBroadcast;
}

void CSceneGraph::SetCurSelectEntityId(int p_entityId)
{
	if (p_entityId < -1 || p_entityId > GetEntities()->size())
	{
		return;
	}

	m_selectionBroadcast->Broadcast(this, p_entityId);
}

CEntity::CEntity(std::string p_name)
{
	m_dirty						= false;
	m_id						= CSceneGraph::RegisterEntity(this);
	m_name						= p_name + "_" + std::to_string(m_id);
}

void CEntity::SetTransform(nm::Transform p_transform)
{
	m_dirty						= true;
	m_transform					= p_transform;
}

CDebugData::CDebugData()
{
	m_drawSubmeshBBox			= false;
	m_drawBBox					= false;
}