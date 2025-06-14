layout(push_constant) uniform MeshID
{
	uint mesh_id;
	uint material_id;
}g_pushConstant;

struct MeshData
{
	mat4  modelMatrix;			// model matrix for this vertex buffer
	mat4  normalMatrix;			// inverse transpose of (view * model)
};
layout(set = 1, binding = 0) uniform Mesh
{
	MeshData data[1];
} g_meshUniform;

layout(set = 1, binding = 1) uniform samplerCube g_env_specular_Sampler;
layout(set = 1, binding = 2) uniform samplerCube g_env_diffuse_Sampler;
layout(set = 1, binding = 3) uniform texture2D g_brdf_lut;

struct Material
{
	vec3 emissive;
	float metallic;
	vec3 pbr_color;
	float roughness;
	uint color_id;
	uint normal_id;
	uint roughMetal_id;
	uint emissive_id;
};
layout(set = 1, binding = 4) buffer Material_Storage
{
	Material data[1];			// list of all material
} g_materials;

struct Light
{
	uint  type_castShadow;
	float color[3];
	float intensity;
	float vector3[3];
	float viewProj[16];
};

layout(set = 1, binding = 5) buffer Light_Storage
{
	uint count;
	Light lights[];
} g_lights;

layout(set = 1, binding = 6) uniform texture2D g_textures[];

vec4 GetColor(uint color_id, vec2 uv)
{
	vec4 color;
	if(color_id < MAX_SUPPORTED_TEXTURES)
	{
		color = texture(sampler2D(g_textures[color_id], g_LinearSampler), uv);
	}
	else
	{
		color = texture(sampler2D(g_textures[DEFAULT_TEXTURE_ID], g_LinearSampler), uv);
	}
	return color;
}

vec3 GetNormal(mat3 TBN, uint normal_id, vec2 uv, vec3 inNormal)
{
	vec3 normal;
	if(normal_id < MAX_SUPPORTED_TEXTURES)
	{
		vec2 xy = (texture(sampler2D(g_textures[normal_id], g_LinearSampler), uv).xy * 2.0) - vec2(1.0);
		float z = sqrt(1.0 - dot(xy, xy));
		normal = vec3(xy, z);
		normal = normalize(TBN * normal);
	}
	else
	{
		normal = normalize(inNormal);
	}

	return normal;
}

vec2 GetRoughMetalPBR(uint roughMetal_id, vec2 uv, vec2 inRoughMetal)
{
	vec2 roughMetal;
	if(roughMetal_id < MAX_SUPPORTED_TEXTURES)
	{
		roughMetal = texture(sampler2D(g_textures[roughMetal_id], g_LinearSampler), uv).yz;
	}
	else
	{
		roughMetal = inRoughMetal;
	}

	return roughMetal;
}

vec3 GetEmissive(Material mat, vec2 uv)
{
	return mat.emissive * 3.0 *  texture(sampler2D(g_textures[mat.emissive_id], g_LinearSampler), uv).xyz;
}