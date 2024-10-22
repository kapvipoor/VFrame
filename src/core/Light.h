#ifndef LIGHT_H
#define LIGHT_H

#include "SceneGraph.h"

#include <string>
#include <vector>

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

	virtual void Show(CVulkanRHI* p_rhi) override;

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

	virtual void Show(CVulkanRHI* p_rhi) override;

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

	virtual void Show(CVulkanRHI* p_rhi) override;

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

#endif