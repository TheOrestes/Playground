#version 450

layout (location = 0) in vec3 vs_outPosition;
layout (location = 0) out vec4 outColor;
layout (binding = 0) uniform sampler2D samplerHDRI;

#define PI 3.1415926535897932384626433832795

vec2 invATan = vec2(0.1591f, 0.3183f);

//---------------------------------------------------------------------------------------------------------------------
vec2 SampleSphericalMap(vec3 v)
{
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= invATan;
	uv += 0.5f;

	return uv;
}

//---------------------------------------------------------------------------------------------------------------------
void main()
{
	vec3 N = normalize(vs_outPosition);
	vec2 uv = SampleSphericalMap(N);

	vec4 hdriColor = texture(samplerHDRI, uv);

	outColor = hdriColor;
}