#ifndef LIGHT_H
#define LIGHT_H

#include "Camera.h"

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
    - Is Ortho/Perspective
    - Logically has Culling feature
    - Frustum exists only as under a camera (for now); later as a math tool (then will behave a lot like a Volume)

    Directional Light Source
    =========================
    - Is an entity with transform
    - Difficult to make BBox from Volume; and best suited as a frustum. Does not need to be a volume
    - Best to perform Culling on the bbox
    - require frustum for rendering

    Point Light Source
    ===================
    - Is an entity with transform
    - Best suited as a volume but needs to be a frustum as well
    - Best to perform culling on bsphere
    - require frustum for rendering but need 6 of them for cubemap

    - Entity has a bbox
    - Volume also has bbox and transform that is borrowed from Entity

    Light is a Volume.
    Light has frustum(s).
    Volume can have a simple check for in or out.
    Scene Graph Culling logic can be applied depending on light type.
    }
*/

class CLight : public CEntity
{
public:
    CLight(std::string p_name);
    ~CLight() {};

    virtual bool Init(const CCamera::InitData&) = 0;
    virtual bool Update(const CCamera::UpdateData&) = 0;
    virtual void SetTransform(nm::Transform p_transform) = 0;

    bool IsCastsShadow() { return m_castShadow; }
    nm::float3 GetColor() { return m_color; }
    float GetIntensity() { return m_intensity; }

private:
    bool m_castShadow;
    nm::float3 m_color;
    float m_intensity;
};

class CDirectionaLight : public CLight
{
public:
    CDirectionaLight(std::string p_name);
    ~CDirectionaLight();

    virtual bool Init(const CCamera::InitData&) override;
    virtual bool Update(const CCamera::UpdateData&) override;
    virtual void SetTransform(nm::Transform p_transform);

    COrthoCamera* GetShadowCamera() { return m_camera; }

private:
    COrthoCamera* m_camera;
    float direction;
};


//class CPointLight : public CLight
//{
//public:
//    CPointLight(std::string p_name);
//    ~CPointLight() {}
//
//    //virtual bool Init(CCamera::InitData) override { return true; };
//    virtual void SetTransform(nm::Transform p_transform) override;
//
//private:
//    //CPerspectiveCamera m_camera[6];
//    float position;
//};


#endif
