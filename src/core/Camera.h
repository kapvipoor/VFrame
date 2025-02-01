#pragma once

#define M_PI 3.141592653589793238

//#include "SceneGraph.h"
#include "external/NiceMath.h"

class CCamera// : public CEntity
{
public:
    struct InitData
    {
        nm::float4                  lookFrom;
        nm::float3                  lookAt;
        nm::float3                  up;
        float                       yaw;
        float                       pitch;
        
        virtual void Pure() = 0;
    };

    struct UpdateData
    {
        float                       timeDelta;
        int                         mouseDelta[2];
        bool                        moveCamera;
        bool                        A;
        bool                        W;
        bool                        D;
        bool                        S;
        bool                        Q;
        bool                        E;
        bool                        Shft;
        nm::Transform               transform;   // if attached to a light source
        float                       yaw;
        float                       pitch;
        float                       roll;
        nm::float4x4                jitter;
        UpdateData()
            : timeDelta(0.0f)
            , mouseDelta{-1, -1}
            , moveCamera(false)
            , A(false)
            , W(false)
            , D(false)
            , S(false)
            , Q(false)
            , E(false)
            , Shft(false)
            , transform()
            , yaw(0.0f)
            , pitch(0.0f)
            , roll(0.0f)
            , jitter(nm::float4x4::identity()) {
        }
    };

    CCamera(/*std::string p_name*/);
    ~CCamera() {};

    virtual bool Init(InitData*);
    virtual void Update(UpdateData data);

    void Look(float p_yaw, float p_pitch, nm::float4 p_lookFrom);

    const nm::float4x4 GetViewProj() const { return m_viewProj; }
    const nm::float4x4 GetJitteredViewProj() const { return m_jitteredViewProj; }
    const nm::float4x4 GetPreViewProj() const { return m_preViewProj; }
    const nm::float4x4 GetInvViewProj() const { return m_invViewProj; }
    const nm::float4x4 GetView() const { return m_view; }
    const nm::float4x4 GetProjection() const { return m_projection; }
    nm::float3 GetLookFrom() const { return m_lookFrom.xyz(); }
    const nm::float3 GetLookAt() const { return m_lookAt; }
    const float GetPitch() const { return m_pitch; }
    const float GetYaw() const { return m_yaw; }

protected:
    bool                            m_bInverted;
    const nm::float3                c_up = nm::float3(0.0f, -1.0f * (m_bInverted ? -1.0f : 1.0f), 0.0f);
    float                           m_dist;
    float                           m_zNear;
    float                           m_zFar;

    float                           m_speed = 1.0f;
    float                           m_yaw = 0.0f;
    float                           m_pitch = 0.0f;
    float                           m_moveScale;

    nm::float3                      m_up;
    nm::float4                      m_lookFrom;
    nm::float3                      m_lookAt;

    nm::float4x4                    m_view;
    nm::float4x4                    m_projection;
    nm::float4x4                    m_viewProj;
    nm::float4x4                    m_jitteredViewProj;
    nm::float4x4                    m_invViewProj;
    nm::float4x4                    m_preViewProj;

    nm::float4 PolarToVector(float yaw, float pitch);
    nm::float4x4 LookAtRH(nm::float4 eyePos, nm::float4 lookAt);
    nm::float4 MoveWASD(UpdateData pdata);
};

class CPerspectiveCamera : public CCamera
{
public:
    struct PerpspectiveInitdData : public InitData
    {
        float                     fov;
        float                     aspect;
        float                     aperture;
        float                     focusDistance;

        virtual void Pure() override {};
    };

    //CPerspectiveCamera(std::string p_entityName) : CCamera(p_entityName) {};
    CPerspectiveCamera() {};
    ~CPerspectiveCamera() {};

    const float GetLensRadius() const { return m_lensRadius; }
    const nm::float3& GetCamX() const { return m_camX; }
    const nm::float3& GetCamY() const { return m_camY; }
    const nm::float3& GetCamZ() const { return m_camZ; }
    const nm::float3& GetLowerLeft() const { return m_lowerLeft; }
    const nm::float3& GetHorizontal() const { return m_horizontal; }
    const nm::float3& GetVetical() const { return m_vertical; }

    void Move(nm::float3 p_lookFrom);
    virtual bool Init(InitData* p_initData) override;
    virtual void Update(UpdateData data) override;

private:
    float                       m_lensRadius;  // for DOF
    float                       m_halfHeight;
    float                       m_halfWidth;
    float                       m_aspect;
    float                       m_vFov;

    nm::float3                  m_lowerLeft;
    nm::float3                  m_vertical;
    nm::float3                  m_horizontal;

    nm::float3                  m_camX;
    nm::float3                  m_camY;
    nm::float3                  m_camZ;

    void LookAt(nm::float4 eyePos, nm::float4 lookAt);
};

class COrthoCamera : public CCamera
{
public:
    struct OrthInitdData : public InitData
    {
        nm::float4              lrbt;
        float                   nearPlane;
        float                   farPlane;

        virtual void Pure() override {};
    };

    COrthoCamera(/*std::string p_entityName*/);
    ~COrthoCamera() {}

    virtual bool Init(InitData* p_initData) override;
    virtual void Update(UpdateData p_data) override;

private:
    nm::float4                  m_lrbt;  // left, right, bottom, top
};