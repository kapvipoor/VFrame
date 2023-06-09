#include "Light.h"

CLight::CLight(std::string p_name)
	:CEntity(p_name)
{
}

//CPointLight::CPointLight(std::string p_name)
//	: CLight(p_name)
//{
//	m_drawAsSphere = true;
//}
//
//void CPointLight::SetTransform(nm::Transform p_transform)
//{
//	m_dirty			= true;
//	
//	// for point lights; we want to
//	// 1. neutralize rotation matrix
//	p_transform.SetRotation(nm::float4x4::identity());
//
//	// 2. uniformalize scaling matrix so that scaling happens in all axis uniformly
//	nm::float3 scale = p_transform.GetScaleVector();
//	float scaleFactor = (scale[0] == scale[1]) ? scale[2] : ((scale[1] == scale[2]) ? scale[0] : ((scale[0] == scale[2]) ? scale[1] : 1.0f));
//	p_transform.SetScale(nm::float4x4::identity() * scaleFactor);
//
//	m_transform		= p_transform;
//	CSceneGraph::RequestSceneBBoxUpdate();
//}

CDirectionaLight::CDirectionaLight(std::string p_name)
	:CLight(p_name)
{
    m_camera = new COrthoCamera();
}

CDirectionaLight::~CDirectionaLight()
{
	delete m_camera;
}

bool CDirectionaLight::Init(const CCamera::InitData& p_data)
{	    
    // Re-initialize the shadow camera's view and projection matrix because we want the 
    // shadow camera to properly fit the new scene bounds
    RETURN_FALSE_IF_FALSE(m_camera->Init(const_cast<COrthoCamera::InitData*>(&p_data)));

    /// Recompute the bounding box of the shadow camera for debug visibility
    // the z values in vulkan ranges between 0-1 and not -1 and 1
    // so, min.z = 0 and max.z = 1
    BBox box = BBox(BBox::Custom, BBox::CameraStyle, nm::float3(-1.0f, -1.0f, 0.0f), nm::float3(1.0f, 1.0f, 1.0f));
    
    // Convert this bbox from View space to world space by multiplying with inverse view proj matrix
    box = box * nm::inverse(m_camera->GetViewProj());

    SetBoundingBox(box, false /*do not recalculate the bounding box of the scene graph*/);

    //m_transform.SetRotation(0.0f, -m_camera->GetPitch(), m_camera->GetYaw());
    //m_transform.SetTranslate(nm::float4(m_camera->GetLookFrom(), 1.0f));

	return true;
}

bool CDirectionaLight::Update(const CCamera::UpdateData& p_data)
{
    
    if (CEntity::m_dirty)
    {
        CCamera::UpdateData camUpdateData = p_data;
        camUpdateData.transform = m_transform;
        m_camera->Update(camUpdateData);
        CEntity::m_dirty = false;
    }

    return true;
}

void CDirectionaLight::SetTransform(nm::Transform p_transform)
{
    // we do not expect directional light to trigger
    // re-computation of scene bounding box
    CEntity::SetTransform(p_transform, false);
}



 
/*
Initialization
1. Construct the Directional light with its shadow camera the way we want it (looking downwards)

Update - Scene change
1. The Light's entity transform remains unchanged
2. If the scene bounds change, we want the shadow camera to recompute its bounding box.

Update - Gizmo interaction with Directional Light
1. Only rotation is valid, do not allow translation or scaling
2. When rotation occurs, update the transform of the entity (the bounding box of the entity will apply as is)
3. Apply transform to shadow camera 




*/