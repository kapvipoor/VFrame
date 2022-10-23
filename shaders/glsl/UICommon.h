layout(push_constant) uniform UIProjection
{
	vec2 translate;
	vec2 scale;
}g_pushConstant;

layout(set = 1, binding = 0) uniform texture2D g_uiTexture[];