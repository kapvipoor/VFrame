layout(push_constant) uniform MeshID
{
	uint mesh_id;
	uint material_id;
}g_pushConstant;

struct MeshData
{
	mat4  model;				// model matrix for this vertex buffer
	mat4  trans_inv_model;
};
layout(set = 1, binding = 0) uniform Mesh
{
	MeshData data[1];
} g_meshUniform;

layout(set = 1, binding = 1) uniform samplerCube g_skyboxSampler;

struct Material
{
	uint color_id;
	uint normal_id;
};
layout(set = 1, binding = 2) buffer Material_Storage
{
	Material data[1];			// list of all material
} g_materials;

layout(set = 1, binding = 3) uniform texture2D g_textures[];