#pragma once

#include <vector>
#include <string>

#include "AssetLoader.h"

#include "external/NiceMath.h"


class CEntity;
class CSceneGraph;
class CSelectionBroadcast;

class CSelectionListener
{
	friend class CSelectionBroadcast;
public:
	CSelectionListener();
	~CSelectionListener() {};

	//virtual void OnSelectionChange(CEntity*) = 0;

	CEntity* GetCurSelectEntity() { return m_selectedEntity; }

protected:
	CEntity* m_selectedEntity;
};

class CSelectionBroadcast
{
	friend class CSelectionListener;
public:
	CSelectionBroadcast() {};
	~CSelectionBroadcast();

	void Broadcast(CSceneGraph* p_sceneGraph, int p_entityId);

private:
	static std::vector<CSelectionListener*> m_listeneers;
};

class CSceneGraph
{
	friend class CEntity;
public:
	typedef std::vector<CEntity*> EntityList;

	CSceneGraph();
	~CSceneGraph();

	static EntityList* GetEntities() { return &s_entities; };

	void SetCurSelectEntityId(int p_id);

private:
	CSelectionBroadcast*		m_selectionBroadcast;
	static EntityList			s_entities;

	static uint32_t RegisterEntity(CEntity* p_entity)
	{
		uint32_t count = (uint32_t)s_entities.size();
		s_entities.push_back(p_entity);
		return count;
	}
};

class CEntity
{
public:
	CEntity(std::string p_name)
	{
		m_id						= CSceneGraph::RegisterEntity(this);
		m_name						= p_name + "_" + std::to_string(m_id);
		m_transform					= nm::float4x4::identity();
	}

	~CEntity() {}

	void SetTransform(nm::float4x4 p_transform) { m_transform = p_transform; }

	int	GetId() { return m_id; }
	const char* GetName() { return m_name.c_str(); }
	nm::float4x4 GetTransform() { return m_transform; }

protected:
	uint32_t					m_id;
	std::string					m_name;
	nm::float4x4				m_transform;

	BBox						m_boundingBox;

};
