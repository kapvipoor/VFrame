#include "Light.h"

CLight::CLight(std::string p_name, Type p_type, float p_intensity, bool p_castShadow)
	:CEntity(p_name)
	, m_type(p_type)
	, m_castShadow(p_castShadow)
	, m_intensity(p_intensity)
	, m_color(nm::float3(1.0f))
{
	m_transform.SetScale(nm::float3(m_intensity));
}

void CLight::SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox)
{
	m_dirty = true;
	m_transform = p_transform;
}

void CLight::Show(CVulkanRHI* p_rhi)
{
	CEntity::Show(p_rhi);

	ImGui::Indent();
	ImGui::Text("Light Type: %s", (m_type == Type::Directional ? "Directional" : "Point"));
	ImGui::Text("Cast Shadow: %s", (m_castShadow == true ? "true" : "false"));
	ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 20.0f);
	ImGui::InputFloat3("Color ", &m_color[0]);
	ImGui::Unindent();
}

CDirectionaLight::CDirectionaLight(std::string p_name, bool p_castShadow, nm::float3 p_direction, float p_intensity, nm::float3 color)
	:CLight(p_name, Type::Directional, p_intensity, p_castShadow)
	, m_direction(p_direction)
{
	m_color = color;
	m_camera = new COrthoCamera();
	m_boundingVolume = new BFrustum(nm::float4x4::identity());
}

CDirectionaLight::~CDirectionaLight()
{
	delete m_boundingVolume;
	delete m_camera;
}

bool CDirectionaLight::Init(const CCamera::InitData& p_data)
{
	// Initialize the shadow camera's view and projection matrix because we
	// want the shadow camera to properly fit the new scene bounds
	RETURN_FALSE_IF_FALSE(m_camera->Init(const_cast<COrthoCamera::InitData*>(&p_data)));

	return true;
}

bool CDirectionaLight::Update(const CCamera::UpdateData& p_data, const CSceneGraph* p_sceneGraph)
{
	if (m_dirty)
	{
		m_direction = (m_transform.GetRotate() * nm::float4(0.0f, 1.0f, 0.0f, 1.0f)).xyz();
		m_intensity = m_transform.GetScaleVector()[0];
	}

	if (m_castShadow && p_sceneGraph)
	{
		// if the scene bounds have changed (this happens when an entity moves
		// out of scene bounds in the previous frame) sunlight's camera is
		// re-initialized with new scene bounding metrics. Their view and
		// projection matrices are recalculated
		// If there is a change in light's direction, we need to recompute the scene fitting
		// because we want an axis aligned bounding frustum around the scene.
		bool recomputeSceneFitting =
			m_dirty ||
			(p_sceneGraph->GetSceneStatus() == CSceneGraph::SceneStatus::ss_BoundsChange);

		if (recomputeSceneFitting)
		{
			const BBox* sceneBB = p_sceneGraph->GetBoundingBox();

			BBox sceneFittingBB = *p_sceneGraph->GetBoundingBox();
			sceneFittingBB = sceneFittingBB * m_transform.GetRotate();
			sceneFittingBB.Merge(*sceneBB);

			float sceneWidth = sceneFittingBB.GetWidth();
			float sceneHeight = sceneFittingBB.GetHeight();
			float sceneDepth = sceneFittingBB.GetDepth();

			nm::float3 sunLightDirection = nm::normalize(GetDirection());

			COrthoCamera::OrthInitdData initData{};
			initData.lookFrom = nm::float4(0.0f, 0.0f, 0.0f, 1.0f);
			initData.lrbt = nm::float4{ -sceneWidth / 2.0f, sceneWidth / 2.0f, -sceneDepth / 2.0f, sceneDepth / 2.0f };
			initData.nearPlane = 0.0f;
			initData.farPlane = sceneHeight;
			initData.yaw = atan2(sunLightDirection[0], sunLightDirection[2]);
			initData.pitch = asin(sunLightDirection[1]);
			m_camera->Init(static_cast<CCamera::InitData*>(&initData));

			//Considering the scene ranges from (-1,-1,-1) to (1,1,1), these
			//offsets ensure the camera is looking from (0,0,-1)
			nm::float3 sceneTranslationOffset = sceneBB->GetUnitBBoxTransform().GetTranslateVector();
			nm::float3 cameraTranslationOffset = m_camera->GetLookAt() * sceneHeight / 2.0f;
			nm::float3 shadowCamLookingFrom = sceneTranslationOffset + cameraTranslationOffset;

			nm::Transform transform;
			transform.SetTranslate(nm::float4(shadowCamLookingFrom, 1.0f));

			CCamera::UpdateData cdata;
			cdata.timeDelta = p_data.timeDelta;
			cdata.transform = transform;

			m_camera->Update(cdata);

			BFrustum newFrustum(nm::inverse(m_camera->GetViewProj()));
			delete m_boundingVolume;
			m_boundingVolume = new BFrustum(newFrustum);

			// reconstructing the light direction from camera's look at
			m_direction = m_camera->GetLookAt();
		}
	}

	m_dirty = false;
	return true;
}

void CDirectionaLight::SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox)
{
	// we do not expect directional light to trigger re-computation of scene
	// bounding box; hence not calling CSceneGraph::RequestSceneBBoxUpdate();
	CLight::SetTransform(p_transform, p_bRecomputeSceneBBox);
}

void CDirectionaLight::Show(CVulkanRHI* p_rhi)
{
	CLight::Show(p_rhi);

	ImGui::Indent();
	ImGui::InputFloat3("Direction ", &m_direction[0]);
	ImGui::Unindent();
}

/*
Initialization
1. Construct the Directional light with its shadow camera the way we want it (looking downwards)

Update - Scene change
1. The Light's entity transform remains unchanged
2. If the scene bounds change, we want the shadow camera to recompute its bounding box.

Update - Guizmo interaction with Directional Light
1. Only rotation is valid, do not allow translation or scaling
2. When rotation occurs, update the transform of the entity (the bounding box of the entity will apply as is)
3. Apply transform to shadow camera
*/

CPointLight::CPointLight(std::string p_name, bool p_castShadow, nm::float3 p_position, float intensity, nm::float3 color)
	: CLight(p_name, Type::Point, intensity, p_castShadow)
	, m_position(p_position)
{
	CEntity::m_transform.SetTranslate(nm::float4(p_position, 1.0f));
	CEntity::m_transform.SetScale(nm::float3(intensity));
	m_color = color;
	m_boundingVolume = new BSphere();
}

CPointLight::~CPointLight()
{
	delete m_boundingVolume;
}

bool CPointLight::Init(const CCamera::InitData&)
{
	return true;
}

bool CPointLight::Update(const CCamera::UpdateData&, const CSceneGraph*)
{
	if (CEntity::m_dirty)
	{
		m_position = m_transform.GetTranslateVector();

		// assuming uniform scaling on all axis.
		m_intensity = m_transform.GetScaleVector()[1];

		CEntity::m_dirty = false;
	}

	return true;
}

void CPointLight::SetTransform(nm::Transform p_transform, bool p_bRecomputeSceneBBox)
{
	// assuming uniform scaling on all axis. We are applying the scaling on the
	// axis that has changed from last update to all the axises
	float oldIntensity = m_intensity;
	nm::float3 scale = p_transform.GetScaleVector();
	m_intensity = (scale[0] != m_intensity) ? scale[0] :
		(scale[1] != m_intensity) ? scale[1] :
		(scale[2] != m_intensity) ? scale[2] : m_intensity;

	if (oldIntensity != m_intensity)
	{
		// updating the transform to reflect that
		p_transform.SetScale(nm::float3(m_intensity));
	}

	CLight::SetTransform(p_transform);

	// we expect point light to trigger re-computation of scene 
	// bounding box if it goes out of scene bounds
	CSceneGraph::RequestSceneBBoxUpdate();
}

void CPointLight::Show(CVulkanRHI* p_rhi)
{
	CLight::Show(p_rhi);

	ImGui::Indent();
	ImGui::InputFloat3("Position ", &m_position[0]);
	ImGui::Unindent();
}

CLights::CLights()
	: m_isDirty(false)
{
	CreateLight(CLight::Type::Directional, "Sunlight", true, nm::float3(1.0f, 1.0f, 0.99f), 10.0f, nm::float3(0.0f, 1.0f, 0.0f));
	CreateLight(CLight::Type::Point, "PointLight_A", false, nm::float3(1.0f, 1.0f, 0.0f), 1.0f, nm::float3(0.0f, 0.0f, 0.0f));
	CreateLight(CLight::Type::Point, "PointLight_B", false, nm::float3(0.0f, 1.0f, 1.0f), 1.0f, nm::float3(1.0f, 0.0f, 0.0f));
	CreateLight(CLight::Type::Point, "PointLight_C", false, nm::float3(1.0f, 0.0f, 1.0f), 1.0f, nm::float3(0.0f, 0.0f, 1.0f));
}

CLights::~CLights()
{
	DestroyLights();
}

void CLights::Update(const CCamera::UpdateData& p_updateData, const CSceneGraph* p_sceneGraph)
{
	int  i = 0;
	bool hasSceneBoundsChanged = (p_sceneGraph->GetSceneStatus() == CSceneGraph::SceneStatus::ss_BoundsChange);
	for (auto& light : m_lights)
	{
		if (hasSceneBoundsChanged || light->IsDirty())
		{
			light->Update(p_updateData, p_sceneGraph);

			nm::float3 vector3 = (light->GetType() == CLight::Type::Directional) ?
				static_cast<CDirectionaLight*>(light)->GetDirection() :
				static_cast<CPointLight*>(light)->GetPosition();

			m_rawGPUData[i].intensity = light->GetIntensity();
			std::copy(std::begin(vector3.data), std::end(vector3.data), std::begin(m_rawGPUData[i].vector3));

			if (light->IsCastsShadow())
			{
				CDirectionaLight* dLight = dynamic_cast<CDirectionaLight*>(light);
				if (dLight)
				{
					float* lightViewProj = const_cast<float*>(&dLight->GetShadowCamera()->GetViewProj().column[0][0]);
					std::copy(&lightViewProj[0], &lightViewProj[16], std::begin(m_rawGPUData[i].viewProj));
				}
			}
			m_isDirty = true;
		}
		i++;
	}
}

void CLights::CreateLight(CLight::Type p_type, const char* p_name, bool p_castShadow, nm::float3 p_color, float p_intensity, nm::float3 p_vector3)
{
	CLight* light = nullptr;
	COrthoCamera::OrthInitdData oData{};
	CPerspectiveCamera::PerpspectiveInitdData pData{};

	switch (p_type)
	{
	case CLight::Type::Directional:
		light = new CDirectionaLight(p_name, p_castShadow, p_vector3 /*direction*/, p_intensity, p_color);
		light->Init(oData);
		break;

	case CLight::Type::Point:
		light = new CPointLight(p_name, p_castShadow, p_vector3 /*position*/, p_intensity, p_color);
		light->Init(pData);
		break;

	default:
		return;
	}

	light->SetId((uint32_t)m_lights.size());

	m_lights.push_back(light);

	float* identity = const_cast<float*>(&nm::float4x4::identity().column[0][0]);

	LightGPUData gpuData{};
	gpuData.type_castShadow = ((uint32_t)light->GetType() << 16) | (uint32_t)(light->IsCastsShadow());
	gpuData.intensity = p_intensity;
	std::copy(std::begin(p_color.data), std::end(p_color.data), std::begin(gpuData.color));
	std::copy(std::begin(p_vector3.data), &p_vector3[3], std::begin(gpuData.vector3));
	std::copy(&identity[0], &identity[16], std::begin(gpuData.viewProj));
	m_rawGPUData.push_back(gpuData);

	m_isDirty = true;
}

void CLights::DestroyLights()
{
	for (auto& light : m_lights)
	{
		delete light;
	}

	m_lights.clear();
	m_rawGPUData.clear();
}
