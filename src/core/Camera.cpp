#include "Camera.h"
#include "AssetLoader.h"

CCamera::CCamera(/*std::string p_name*/)
//    : CEntity(p_name)
{
    m_bInverted         = false;
    m_moveScale         = 1.0f;
}

bool CCamera::Init(InitData*)
{
//    BBox box = BBox(BBox::Unit, BBox::CameraStyle);
//    // the z values in vulkan ranges between 0-1 and not -1 and 1
//    // so, min.z = 0 and max.z = 1
//    box.bbMax[2] = 1.0f;
//    ComputeBBox(box);
//    for (int i = 0; i < 8; i++)
//    {
//        box.bBox[i] = (nm::inverse(m_viewProj) * nm::float4(box.bBox[i], 1.0)).xyz();
//    }
//    SetBoundingBox(box);
    return true;
}

void CCamera::Update(UpdateData data)
{   
    //if (m_dirty)
    //{
    //    m_lookFrom      = nm::float4(data.transform.GetTranslateVector(), 1.0f);
    //    m_pitch         = -data.transform.GetRotateVector().x();
    //    m_yaw           = data.transform.GetRotateVector().y();
    //    //m_dirty         = false;
    //}
}

nm::float4 CCamera::PolarToVector(float yaw, float pitch)
{
    return  nm::float4(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch), 0);
}

nm::float4x4 CCamera::LookAtRH(nm::float4 eyePos, nm::float4 lookAt)
{
    nm::float3 eyepos_{ eyePos.x(), eyePos.y(), eyePos.z() };
    nm::float3 lookat_{ lookAt.x(), lookAt.y(), lookAt.z() };

    return nm::lookAtRH(eyepos_, lookat_, m_up); // up should be nm::float3(0, 1, 0)
}

nm::float4 CCamera::MoveWASD(UpdateData pdata)
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

void CPerspectiveCamera::Move(nm::float3 p_lookFrom)
{
    m_lookFrom = nm::float4(p_lookFrom, 0.0);
    m_view = m_view * nm::translation(p_lookFrom);
    m_viewProj = m_projection * m_view;
}

bool CPerspectiveCamera::Init(InitData* p_initData)
{
    PerpspectiveInitdData* persInitData = dynamic_cast<PerpspectiveInitdData*>(p_initData);
    if (!persInitData)
        return false;

    {
        m_vFov = persInitData->fov;
        m_aspect = persInitData->aspect;
        m_up = persInitData->up;
        m_lookFrom = nm::float4{ persInitData->lookFrom.xyz(), 1.0f };
        m_lookAt = persInitData->lookAt;

        m_zNear = 0.1f;
        m_zFar = 1000.0f;
        m_dist = 1;
        m_lensRadius = persInitData->aperture / 2.0f;

        m_yaw = persInitData->yaw;
        m_pitch = persInitData->pitch;

        float vFovRad = persInitData->fov * (float)M_PI / 180.f;
        m_halfHeight = tan(vFovRad / 2.0f);
        m_halfWidth = m_aspect * m_halfHeight;

        m_camZ = nm::normalize(persInitData->lookFrom.xyz() - persInitData->lookAt);       //w
        m_camX = nm::normalize(nm::cross(persInitData->up, m_camZ));                       //u
        m_camY = nm::normalize(nm::cross(m_camZ, m_camX));                                 //v

        m_lowerLeft = nm::float3(m_lookFrom.x(), m_lookFrom.y(), m_lookFrom.z())
            - (m_halfWidth * persInitData->focusDistance * m_camX)
            - (m_halfHeight * persInitData->focusDistance * m_camY)
            - (persInitData->focusDistance * m_camZ);

        m_vertical = 2.0f * m_halfHeight * persInitData->focusDistance * m_camY;
        m_horizontal = 2.0f * m_halfWidth * persInitData->focusDistance * m_camX;

        m_projection = nm::perspective(vFovRad, persInitData->aspect, m_zNear, m_zFar);
    }

    CCamera::Init(p_initData);

    return true;
}

void CPerspectiveCamera::Update(UpdateData p_data)
{
    if (p_data.moveCamera == true)
    {
        m_yaw -= p_data.mouseDelta[0] / 654.f;
        m_pitch += p_data.mouseDelta[1] / 654.f;
    }

    m_lookFrom = m_lookFrom + nm::transpose(m_view) * nm::float4(MoveWASD(p_data) * m_speed * (float)p_data.timeDelta);
    nm::float4 dir = PolarToVector(m_yaw, m_pitch) * m_dist;
    LookAt(m_lookFrom, m_lookFrom - dir);

    m_viewProj = m_projection * m_view;

    //std::cout << "Printing View: " << m_view.column[3][0] << ", " << m_view.column[3][1] << ", " << m_view.column[3][2] << std::endl;

    // TODO
    //CCamera::Update(p_data);
}

void CPerspectiveCamera::LookAt(nm::float4 eyePos, nm::float4 lookAt)
{
    // this is unnessary
    m_lookAt                    = nm::float3(lookAt.x(), lookAt.y(), lookAt.z());

    m_lookFrom                  = eyePos;
    m_view                      = LookAtRH(eyePos, lookAt);
    m_dist                      = nm::length(lookAt - eyePos);

    nm::float4x4 inView         = nm::inverse(m_view);
    nm::float4 zBasis           = inView.column[2];
    m_yaw                       = atan2f(zBasis.x(), zBasis.z());
    float fLen                  = sqrtf(zBasis.z() * zBasis.z() + zBasis.x() * zBasis.x());
    m_pitch                     = atan2f(zBasis.y(), fLen);
}

COrthoCamera::COrthoCamera(/*std::string p_entityName*/)
    //: CCamera(p_entityName) 
{
}

bool COrthoCamera::Init(InitData* p_initData)
{
    OrthInitdData* orthoInitData = dynamic_cast<OrthInitdData*>(p_initData);
    if (!orthoInitData)
    {
        std::cout << "dynamic_cast<OrthInitdData*>(p_initData) failed";
        return false;
    }        

    m_lrbt                      = orthoInitData->lrbt;
    m_up                        = c_up;
    m_lookFrom                  = nm::float4{ orthoInitData->lookFrom.xyz(), 1.0f };
    m_zNear                     = orthoInitData->nearPlane;
    m_zFar                      = orthoInitData->farPlane;
    m_dist                      = 1;
    m_yaw                       = orthoInitData->yaw;
    m_pitch                     = orthoInitData->pitch;
    m_projection                = nm::orthoRH_01(m_lrbt.x(), m_lrbt.y(), m_lrbt.z(), m_lrbt.w(), m_zNear, m_zFar); // TODO: there is a problem here with projection generation and some issue with ZNear and ZFar
    nm::float4 dir              = PolarToVector(m_yaw, m_pitch) * m_dist;
    m_lookAt                    = dir.xyz(); // (m_lookFrom - dir).xyz();
    m_view                      = LookAtRH(m_lookFrom, m_lookFrom - dir);
    m_viewProj                  = m_projection * m_view;

    //CCamera::Init(p_initData);

    //m_transform.SetRotation(0.0f, -m_pitch, m_yaw);
    //m_transform.SetTranslate(m_lookFrom);

    return true;
}

void COrthoCamera::Update(UpdateData p_data)
{
    //if (m_dirty)
    {
        //CCamera::Update(p_data);
        //nm::float4x4 rotationMat = p_data.transform.GetRotate();
        //m_yaw = atan2f(rotationMat.column[2][0], rotationMat.column[2][1]);
        //m_pitch = acos(rotationMat.column[2][2]);

        m_lookFrom = nm::float4(p_data.transform.GetTranslateVector(), 1.0f);
        //m_pitch = -p_data.pitch;            //transform.GetRotateVector().x();
        //m_yaw = p_data.yaw;                 // transform.GetRotateVector().y();

        // recalculating view and viewproj
        nm::float4 dir          = PolarToVector(m_yaw, m_pitch) * m_dist;
        m_lookAt                = dir.xyz(); // (m_lookFrom - dir).xyz();
        m_view                  = LookAtRH(m_lookFrom, m_lookFrom - dir);
        m_viewProj              = m_projection * m_view;
        m_up                    = (p_data.transform.GetRotate() * nm::float4(c_up, 1.0f)).xyz();
    }
}
