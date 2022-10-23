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
	int GetSelectedSubMesh() { return m_selectedSubMeshIndex; }

protected:
	CEntity* m_selectedEntity;
	int	m_selectedSubMeshIndex;
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

class CDebugData
{
public:
	CDebugData();
	~CDebugData() {};

	bool IsDebugDrawEnabled() { return m_drawBBox; }
	void SetDebugDrawEnable(bool p_enable) { m_drawBBox = p_enable; }

	bool IsSubmeshDebugDrawEnabled() { return m_drawSubmeshBBox; }
	void SetSubmeshDebugDrawEnable(bool p_enable) { m_drawSubmeshBBox = p_enable; }

protected:
	bool						m_drawBBox;
	bool						m_drawSubmeshBBox;
};
 
class CSceneGraph : CDebugData
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

class CEntity : public CDebugData
{
public:
	CEntity(std::string p_name);
	~CEntity() {}

	int	GetId() { return m_id; }
	const char* GetName() { return m_name.c_str(); }

	void SetTransform(nm::Transform p_transform);
	nm::Transform& GetTransform() { return m_transform; }
	
	void SetBoundingBox(BBox p_bbox) { m_boundingBox = p_bbox; }
	BBox* GetBoundingBox() { return &m_boundingBox; }

	uint32_t GetSubBoundingBoxCount() { return (uint32_t)m_subBoundingBoxes.size(); }
	void SetSubBoundingBox(BBox p_bbox) { m_subBoundingBoxes.push_back(p_bbox); }
	BBox* GetSubBoundingBox(uint32_t p_id) { return &(m_subBoundingBoxes[p_id]); }

	bool IsDirty() { return m_dirty; }
	void SetDirty(bool p_dirty) { m_dirty = p_dirty; }

protected:
	bool						m_dirty;
	uint32_t					m_id;
	std::string					m_name;
	nm::Transform				m_transform;
	BBox						m_boundingBox;
	std::vector<BBox>			m_subBoundingBoxes;

	virtual void forPolymorphism() {};
};