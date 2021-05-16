#version 450

layout(location=0) in vec3 in_Position;

layout(push_constant) uniform PushConsts 
{
	mat4 matMVP;
	float roughness;
	uint numSamples;
} pushConst;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = in_Position;
	gl_Position = pushConst.matMVP * vec4(in_Position, 1.0);
}