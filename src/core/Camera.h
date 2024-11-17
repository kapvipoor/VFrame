#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_CONFIG_CLIP_CONTROL GLM_CLIP_CONTROL_RH_ZO

#include "external/NiceMath.h"
#include "external/glm/glm/vec3.hpp"
#include "external/glm/glm/vec4.hpp"
#include "external/glm/glm/mat4x4.hpp"
#include "external/glm/glm/gtc/matrix_transform.hpp"

#define M_PI 3.141592653589793238

class CCamera// : public CEntity
{
public:
    struct InitData
    {
        glm::vec4                  lookFrom;
        glm::vec3                  lookAt;
        glm::vec3                  up;
        float                      yaw;
        float                      pitch;
        
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
        glm::mat4x4                 jitter;
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
        {
            jitter = glm::identity<glm::mat4x4>();
        }
    };

    CCamera(/*std::string p_name*/);
    ~CCamera() {};

    virtual bool Init(InitData*);
    virtual void Update(UpdateData data);

    const glm::mat4x4 GetViewProj() const { return m_viewProj; }
    const glm::mat4x4 GetView() const { return m_jitteredView; }
    const glm::mat4x4 GetProjection() const { return m_jitteredProjection; }
    glm::vec3 GetLookFrom() const { return glm::vec3(m_lookFrom); }
    const glm::vec3 GetLookAt() const { return m_lookAt; }
    const float GetPitch() const { return m_pitch; }
    const float GetYaw() const { return m_yaw; }

protected:
    bool                            m_bInverted;
    const glm::vec3                 c_up = glm::vec3(0.0f, -1.0f * (m_bInverted ? -1.0f : 1.0f), 0.0f);
    float                           m_dist;
    float                           m_zNear;
    float                           m_zFar;

    float                           m_speed = 1.0f;
    float                           m_yaw = 0.0f;
    float                           m_pitch = 0.0f;
    float                           m_moveScale;

    glm::vec3                      m_up;
    glm::vec4                      m_lookFrom;
    glm::vec3                      m_lookAt;

    glm::mat4x4                    m_view;
    glm::mat4x4                    m_jitteredView;
    glm::mat4x4                    m_projection;
    glm::mat4x4                    m_jitteredProjection;
    glm::mat4x4                    m_viewProj;

    glm::vec4 PolarToVector(float yaw, float pitch);
    glm::mat4x4 LookAtRH(glm::vec4 eyePos, glm::vec4 lookAt);
    glm::vec4 MoveWASD(UpdateData pdata);
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
    const glm::vec3& GetCamX() const { return m_camX; }
    const glm::vec3& GetCamY() const { return m_camY; }
    const glm::vec3& GetCamZ() const { return m_camZ; }
    const glm::vec3& GetLowerLeft() const { return m_lowerLeft; }
    const glm::vec3& GetHorizontal() const { return m_horizontal; }
    const glm::vec3& GetVetical() const { return m_vertical; }

    void Move(glm::vec3 p_lookFrom);
    virtual bool Init(InitData* p_initData) override;
    virtual void Update(UpdateData data) override;

private:
    float                       m_lensRadius;  // for DOF
    float                       m_halfHeight;
    float                       m_halfWidth;
    float                       m_aspect;
    float                       m_vFov;

    glm::vec3                  m_lowerLeft;
    glm::vec3                  m_vertical;
    glm::vec3                  m_horizontal;

    glm::vec3                  m_camX;
    glm::vec3                  m_camY;
    glm::vec3                  m_camZ;

    void LookAt(glm::vec4 eyePos, glm::vec4 lookAt);
};

class COrthoCamera : public CCamera
{
public:
    struct OrthInitdData : public InitData
    {
        glm::vec4              lrbt;
        float                  nearPlane;
        float                  farPlane;

        virtual void Pure() override {};
    };

    COrthoCamera(/*std::string p_entityName*/);
    ~COrthoCamera() {}

    virtual bool Init(InitData* p_initData) override;
    virtual void Update(UpdateData p_data) override;

private:
    glm::vec4                  m_lrbt;  // left, right, bottom, top
};