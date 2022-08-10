#pragma once

#define M_PI 3.141592653589793238

#include "SceneGraph.h"
#include "external/NiceMath.h"

class CCamera : public CEntity
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
    };

    CCamera(std::string p_name)
    :   CEntity(p_name)
    {
        m_moveScale = 1.0f;
    };
    ~CCamera() {};

    virtual bool Init(InitData*) = 0;
    virtual void Update(UpdateData data) = 0;

    const nm::float4x4 GetViewProj() const { return m_viewProj; }
    const nm::float4x4 GetView() const { return m_view; }
    const nm::float4x4 GetProjection() const { return m_projection; }
    const nm::float3 GetLookFrom() const { return m_lookFrom.xyz(); }
    const nm::float3 GetLookAt() const { return m_lookAt; }

protected:
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

    nm::float4 PolarToVector(float yaw, float pitch)
    {
        return  nm::float4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
    }

    nm::float4x4 LookAtRH(nm::float4 eyePos, nm::float4 lookAt)
    {
        nm::float3 eyepos_{ eyePos.x(), eyePos.y(), eyePos.z() };
        nm::float3 lookat_{ lookAt.x(), lookAt.y(), lookAt.z() };

        return nm::lookAtRH(eyepos_, lookat_, m_up); // up should be nm::float3(0, 1, 0)
    }

    nm::float4 MoveWASD(UpdateData pdata)
    {
        float move = 10.0f;
        float x = 0, y = 0, z = 0;

        if (pdata.W)
        {
            z = -move;
        }
        if (pdata.S)
        {
            z = move;
        }
        if (pdata.A)
        {
            x = -move;
        }
        if (pdata.D)
        {
            x = move;
        }
        if (pdata.E)
        {
            y = move;
        }
        if (pdata.Q)
        {
            y = -move;
        }

        pdata.Shft ? (m_moveScale *= 1.01f) : (m_moveScale = 0.25f);

        return nm::float4(x, y, z, 0.0f) * m_moveScale;
    }
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

    CPerspectiveCamera(std::string p_entityName) : CCamera(p_entityName) {};

    ~CPerspectiveCamera() {};

    const float GetLensRadius() const { return m_lensRadius; }
    const nm::float3& GetCamX() const { return m_camX; }
    const nm::float3& GetCamY() const { return m_camY; }
    const nm::float3& GetCamZ() const { return m_camZ; }
    const nm::float3& GetLowerLeft() const { return m_lowerLeft; }
    const nm::float3& GetHorizontal() const { return m_horizontal; }
    const nm::float3& GetVetical() const { return m_vertical; }

    void Move(nm::float3 p_lookFrom) 
    { 
        m_lookFrom              = nm::float4(p_lookFrom, 0.0); 
        m_view                  = m_view * nm::translation(p_lookFrom);
        m_viewProj              = m_projection * m_view;
    };

    virtual bool Init(InitData* p_initData) override
    {
        PerpspectiveInitdData* persInitData = dynamic_cast<PerpspectiveInitdData*>(p_initData);
        if (!persInitData)
            return false;

        {
            m_vFov              = persInitData->fov;
            m_aspect            = persInitData->aspect;
            m_up                = persInitData->up;
            m_lookFrom          = nm::float4{ persInitData->lookFrom.xyz(), 1.0f};
            m_lookAt            = persInitData->lookAt;

            m_zNear             = 0.1f;
            m_zFar              = 1000.0f;
            m_dist              = 1;
            m_lensRadius        = persInitData->aperture / 2.0f;

            m_yaw               = persInitData->yaw;
            m_pitch             = persInitData->pitch;

            float vFovRad       = persInitData->fov * (float)M_PI / 180.f;
            m_halfHeight        = tan(vFovRad / 2.0f);
            m_halfWidth         = m_aspect * m_halfHeight;

            m_camZ              = nm::normalize(persInitData->lookFrom.xyz() - persInitData->lookAt);       //w
            m_camX              = nm::normalize(nm::cross(persInitData->up, m_camZ));                       //u
            m_camY              = nm::normalize(nm::cross(m_camZ, m_camX));                                 //v

            m_lowerLeft         = nm::float3(m_lookFrom.x(), m_lookFrom.y(), m_lookFrom.z())
                                        - (m_halfWidth * persInitData->focusDistance * m_camX)
                                        - (m_halfHeight * persInitData->focusDistance * m_camY)
                                        - (persInitData->focusDistance * m_camZ);

            m_vertical          = 2.0f * m_halfHeight * persInitData->focusDistance * m_camY;
            m_horizontal        = 2.0f * m_halfWidth * persInitData->focusDistance * m_camX;

            m_projection        = nm::perspective(vFovRad, persInitData->aspect, m_zNear, m_zFar);
        }

        return true;
    }

    virtual void Update(UpdateData data) override
    {
        if (data.moveCamera == true)
        {
            m_yaw               -= data.mouseDelta[0] / 654.f;
            m_pitch             += data.mouseDelta[1] / 654.f;
        }
    
        m_lookFrom              = m_lookFrom + nm::transpose(m_view) * nm::float4(MoveWASD(data) * m_speed * (float)data.timeDelta);
        nm::float4 dir          = PolarToVector(m_yaw, m_pitch) * m_dist;
        LookAt(m_lookFrom, m_lookFrom - dir);

        m_viewProj              = m_projection * m_view;
    }

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

    void LookAt(nm::float4 eyePos, nm::float4 lookAt)
    {
        // this is unnessary
        m_lookAt                = nm::float3(lookAt.x(), lookAt.y(), lookAt.z());
       
        m_lookFrom              = eyePos;
        m_view                  = LookAtRH(eyePos, lookAt);
        m_dist                  = nm::length(lookAt - eyePos);

        nm::float4x4 inView     = nm::inverse(m_view);
        nm::float4 zBasis       = inView.column[2];
        m_yaw                   = atan2f(zBasis.x(), zBasis.z());
        float fLen              = sqrtf(zBasis.z() * zBasis.z() + zBasis.x() * zBasis.x());
        m_pitch                 = atan2f(zBasis.y(), fLen);
    }
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

    COrthoCamera(std::string p_entityName) : CCamera(p_entityName) {}
    ~COrthoCamera() {}

    virtual bool Init(InitData* p_initData) override
    {
        OrthInitdData* orthoInitData = dynamic_cast<OrthInitdData*>(p_initData);
        if (!orthoInitData)
            return false;

        {
            m_lrbt              = orthoInitData->lrbt;
            m_up                = orthoInitData->up;
            m_lookFrom          = nm::float4{ orthoInitData->lookFrom.xyz(), 1.0f};
            m_zNear             = orthoInitData->nearPlane;
            m_zFar              = orthoInitData->farPlane;
            m_dist              = 1;

            m_yaw               = orthoInitData->yaw;
            m_pitch             = orthoInitData->pitch;

            m_projection        = nm::orthoRH_01(m_lrbt.x(), m_lrbt.y(), m_lrbt.z(), m_lrbt.w(), m_zNear, m_zFar);

            nm::float4 dir      = PolarToVector(m_yaw, m_pitch)* m_dist;
            m_lookAt            = (m_lookFrom - dir).xyz();
            m_view              = LookAtRH(m_lookFrom, m_lookFrom - dir);
            m_viewProj          = m_projection * m_view;
        }

        return true;
    }

    virtual void Update(UpdateData p_data) override {}

private:
    nm::float4 m_lrbt;  // left, right, bottom, top
};