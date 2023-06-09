#pragma once

#include <vector>
#include <string>

#include "AssetLoader.h"

#include "external/NiceMath.h"

class CPerspectiveCamera;
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

	bool IsDebugDrawEnabled() const { return m_debugDraw; }
	void SetDebugDrawEnable(bool p_enable) { m_debugDraw = p_enable; }

	bool IsSubmeshDebugDrawEnabled() { return m_debugDrawSubmeshes; }
	void SetSubmeshDebugDrawEnable(bool p_enable) { m_debugDrawSubmeshes = p_enable; }

	bool IsDrawAsSpehereEnabled() { return m_drawAsSphere; }
	void SetDrawAsSphereEnable(bool p_enable) { m_drawAsSphere = p_enable; }

protected:
	bool m_debugDraw;
	bool m_debugDrawSubmeshes;
	bool m_drawAsSphere;
};
 
class CSceneGraph : public CDebugData
{
	friend class CEntity;
public:
	typedef std::vector<CEntity*> EntityList;

	enum SceneStatus
	{
			ss_NoChange = 0
		,	ss_SceneMoved				// the submeshes have moved but have not changed the bounds of the scene
		,	ss_BoundsChange				// the submeshes have moved and have chnaged the bounds of the scene
	};

	CSceneGraph(CPerspectiveCamera*);
	~CSceneGraph();

	bool Update();

	static void RequestSceneBBoxUpdate();

	static EntityList* GetEntities() { return &s_entities; };
	void SetCurSelectEntityId(int p_id);

	const BBox* GetBoundingBox() const { return &m_boundingBox; }

	nm::Transform GetTransform() const { return s_sceneTransform; }
	SceneStatus GetSceneStatus() { return m_sceneStatus; }

	CPerspectiveCamera* GetPrimaryCamera() { return m_primaryCamera; }

private:
	static EntityList			s_entities;
	static bool					s_bShouldRecomputeSceneBBox;
	static nm::Transform		s_sceneTransform;				// this is only used by the entity to multiply with its transform 
	
	CPerspectiveCamera*			m_primaryCamera;
	
	BBox						m_boundingBox;
	CSelectionBroadcast*		m_selectionBroadcast;
	SceneStatus					m_sceneStatus;

	static uint32_t RegisterEntity(CEntity* p_entity);
};

class CEntity : public CDebugData
{
public:
	CEntity(std::string p_name);
	~CEntity() {}

	int	GetId() { return m_id; }
	const char* GetName() { return m_name.c_str(); }

	virtual void SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox = true);
	nm::Transform& GetTransform();
	
	void SetBoundingBox(BBox p_bbox, bool p_bRecomputeSceneBBox = true);
	BBox* GetBoundingBox() { return &m_boundingBox; }

	uint32_t GetSubBoundingBoxCount() { return (uint32_t)m_subBoundingBoxes.size(); }
	void SetSubBoundingBox(BBox p_bbox) { m_subBoundingBoxes.push_back(p_bbox); }
	BBox* GetSubBoundingBox(uint32_t p_id) { return &(m_subBoundingBoxes[p_id]); }

	//bool IsDirty() { return m_dirty; }
	void SetDirty(bool p_dirty) { m_dirty = p_dirty; }

protected:
	// Entity is dirty when its transform has been changed and has not been applied 
	// to the child classes that inherit the entity class. Once the changed transform 
	// is applied, dirty state is reverted
	bool						m_dirty;
	uint32_t					m_id;
	std::string					m_name;
	nm::Transform				m_transform;
	BBox						m_boundingBox;
	std::vector<BBox>			m_subBoundingBoxes;

	virtual void forPolymorphism() {};
};