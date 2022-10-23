// Trowbridge-Reitz GGX normal distribution function
float DistrubutionGGX(vec3 N, vec3 H, float Roughness)
{
	float roughnessSq	= Roughness * Roughness;
	float a2			= roughnessSq * roughnessSq;
	float NdotH			= max(dot(N, H), 0.0);
	float NdotHSq		= NdotH * NdotH;
	float denom			= (NdotHSq * (a2 - 1.0) + 1.0);
	denom				= PI * denom * denom;
	return a2 / denom;
}

// Schlick GGX to statistically estiamte light ray occlusion
float GeometrySchlickGGX(float NdotV, float K)
{
	float denom = (NdotV * (1.0 - K) + K);
	return NdotV / denom;
}

// Applying Schlick GGX on Light vector to compute for geometry shadowing
// Applying SchlickGGX on View vector to compute for geometry occlusion
float GeometrySmith(vec3 N, vec3 V, vec3 L, float Roughness)
{
	float k = (Roughness + 1.0);
	k = k * k;
	k = k / 8.0;
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	return GeometrySchlickGGX(NdotL, k) * GeometrySchlickGGX(NdotV, k);
}

// Fresnel Schlick Approximation for Fresnel Effect
vec3 FresnelSchlick(float cosThetha, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosThetha, 0.0, 1.0), 5.0);
}