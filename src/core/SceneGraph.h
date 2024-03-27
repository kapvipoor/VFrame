#pragma once

#include <vector>
#include <string>

#include "Camera.h"
#include "AssetLoader.h"

#include "external/NiceMath.h"

class CPerspectiveCamera;
class CEntity;
class CSceneGraph;
class CSelectionBroadcast;

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
			uipt_same	= 0		// Adds ui-content to the same panel
		,	uipt_new	= 1		// Adds ui-content to a new panel
	};

	CUIParticipant(ParticipationType pPartType, UIDPanelType pPanelType ,std::string pPanelName = " ");
	~CUIParticipant();

	virtual void Show() = 0;

	ParticipationType GetParticipationType() { return m_participationType; }
	UIDPanelType GetPanelType() { return m_uIDPanelType; }
	std::string GetPanelName() { return m_panelName; }
protected:

	bool m_updated;
	ParticipationType m_participationType;
	UIDPanelType m_uIDPanelType;
	std::string m_panelName;

	bool Header(const char* caption);
	bool CheckBox(const char* caption, bool* value);
	bool CheckBox(const char* caption, int32_t* value);
	bool RadioButton(const char* caption, bool value);
	//bool InputFloat(const char* caption, float* value, float step, uint32_t precision);
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

	void Show();

private:
	static std::vector<CUIParticipant*> m_uiParticipants;

	void BeginPanel(std::string p_panelName);
	void EndPanel();
};

class CSelectionListener
{
	friend class CSelectionBroadcast;
public:
	CSelectionListener();
	~CSelectionListener() {};

	//virtual void OnSelectionChange(CEntity*) = 0;

	CEntity* GetCurSelectEntity() { return m_selectedEntity; }
	int GetSelectedSubMeshId() { return m_selectedSubMeshId; }

protected:
	CEntity* m_selectedEntity;
	int	m_selectedSubMeshId;
};

class CSelectionBroadcast
{
	friend class CSelectionListener;
public:
	CSelectionBroadcast() {};
	~CSelectionBroadcast();

	// Triggered on selection
	void BroadcastEntityId(CSceneGraph* p_sceneGraph, int p_entityId);
	void BroadcastSubmeshId(CSceneGraph* p_sceneGraph, int p_submeshId);

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

protected:
	bool m_debugDraw;
	bool m_debugDrawSubmeshes;
};
 
class CSceneGraph : public CDebugData
{
	friend class CEntity;
public:
	typedef std::vector<CEntity*> EntityList;

	enum SceneStatus
	{
			ss_NoChange = 0
		,	ss_SceneMoved				// the sub-meshes have moved but have not changed the bounds of the scene
		,	ss_BoundsChange				// the sub-meshes have moved and have changed the bounds of the scene
	};

	CSceneGraph(CPerspectiveCamera*);
	~CSceneGraph();

	void Update();

	static void RequestSceneBBoxUpdate();

	static EntityList* GetEntities() { return &s_entities; };
	void SetCurSelectEntityId(int p_id);
	void SetCurSelectedSubMeshId(int p_id);

	const BBox* GetBoundingBox() const { return &m_boundingBox; }

	nm::Transform GetTransform() const { return s_sceneTransform; }
	SceneStatus GetSceneStatus() const { return m_sceneStatus; }

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

class CEntity : public CDebugData, public CUIParticipant
{
public:
	CEntity(std::string p_name);
	~CEntity() {}

	virtual void Show() override;

	int	GetId() { return m_id; }
	const char* GetName() { return m_name.c_str(); }

	virtual void SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox = true) = 0;
	nm::Transform& GetTransform();
	
	void SetBoundingVolume(BVolume* p_bvol, bool p_bRecomputeSceneBBox = true);
	BVolume* GetBoundingVolume() { return m_boundingVolume; }

	bool IsDirty() { return m_dirty; }
	void SetDirty(bool p_dirty) { m_dirty = p_dirty; }

protected:
	// Entity is dirty when its transform has been changed and has not been applied 
	// to the child classes that inherit the entity class. Once the changed transform 
	// is applied, dirty state is reverted
	bool						m_dirty;
	uint32_t					m_id;
	std::string					m_name;
	nm::Transform				m_transform;
	BVolume*					m_boundingVolume;

	virtual void forPolymorphism() {};
};

/*
	Volume
	=======
	- Is 3D space with its transform
	- Has its BBox/BSphere
	- Logically has culling feature
	- Can exist as a mathematical tool - not required for now

	Camera
	=========
	- Has a frustum
	- Is Orthographic/Perspective
	- Logically has Culling feature
	- Frustum exists only as under a camera (for now); later as a math tool (then will behave a lot like a Volume)

	Directional Light Source
	=========================
	- Is an entity with transform
	- Difficult to make BBox from Volume; and best suited as a frustum. Does not need to be a volume
	- Best to perform Culling on the bounding box
	- require frustum for rendering

	Point Light Source
	===================
	- Is an entity with transform
	- Best suited as a volume but needs to be a frustum as well
	- Best to perform culling on bounding sphere
	- require frustum for rendering but need 6 of them for cube-map

	- Entity has a bounding box
	- Volume also has bounding box and transform that is borrowed from Entity

	Light is a Volume.
	Light has frustum(s).
	Volume can have a simple check for in or out.
	Scene Graph Culling logic can be applied depending on light type.
	}
*/
class CLight : public CEntity
{
public:
	enum Type
	{
		Directional = 0
		, Point
	};

	CLight(std::string p_name, Type p_type, float p_intensity, bool p_castShadow);
	~CLight() {};

	virtual bool Init(const CCamera::InitData&) = 0;
	virtual bool Update(const CCamera::UpdateData&, const CSceneGraph*) = 0;
	virtual void SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox = true);

	virtual void Show() override;

	Type GetType() { return m_type; }
	bool IsCastsShadow() { return m_castShadow; }
	nm::float3 GetColor() { return m_color; }
	float GetIntensity() { return m_intensity; }

	void SetId(uint32_t id) { m_id = id; }
	uint32_t GetId() { return m_id; }

protected:
	uint32_t m_id;
	Type m_type;
	bool m_castShadow;
	nm::float3 m_color;
	float m_intensity;
};

class CDirectionaLight : public CLight
{
public:
	CDirectionaLight(std::string p_name, bool p_castShadow, nm::float3 p_direction, float intensity, nm::float3 color);
	~CDirectionaLight();

	virtual bool Init(const CCamera::InitData&) override;
	virtual bool Update(const CCamera::UpdateData&, const CSceneGraph*) override;
	virtual void SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox = true) override;

	virtual void Show() override;

	COrthoCamera* GetShadowCamera() { return m_camera; }
	nm::float3 GetDirection() { return m_direction; }

private:
	COrthoCamera* m_camera;
	nm::float3 m_direction;
};

class CPointLight : public CLight
{
public:
	CPointLight(std::string p_name, bool p_castShadow, nm::float3 p_position, float intensity, nm::float3 color);
	~CPointLight();

	virtual bool Init(const CCamera::InitData&) override;
	virtual bool Update(const CCamera::UpdateData&, const CSceneGraph*) override;

	virtual void SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox) override;

	virtual void Show() override;

	nm::float3 GetPosition() { return m_position; }

private:
	//CPerspectiveCamera m_camera[6];
	nm::float3 m_position;
};

class CLights
{
public:
	struct LightGPUData
	{
		uint32_t type_castShadow;
		float color[3];
		float intensity;
		float vector3[3];
		float viewProj[16];
	};

	CLights();
	~CLights();

	void Update(const CCamera::UpdateData& p_updateData, const CSceneGraph*);
	void CreateLight(CLight::Type p_type, const char* p_name, bool p_castShadow, nm::float3 p_color, float p_intensity, nm::float3 p_position);
	void DestroyLights();

	bool IsDirty() { return m_isDirty; }
	void SetDirty(bool pDirty) { m_isDirty = pDirty; }
	std::vector<LightGPUData> GetLightsGPUData() { return m_rawGPUData; }

private:
	bool m_isDirty;
	std::vector<CLight*> m_lights;
	std::vector<LightGPUData> m_rawGPUData;
};