#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 in_Position;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec3 in_Tangent;
layout(location=3) in vec3 in_BiNormal;
layout(location=4) in vec2 in_UV;

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 matModel;
    mat4 matView;
    mat4 matProjection;
} shaderData;

// NOT IN USE, LEFT FOR REFERENCE
// layout(set = 0, binding = 1) uniform UboModel
// {
//     mat4 matModel;
// } uboModel;

//layout (push_constant) uniform PushModel
//{
//    mat4 matModel;
//} pushModel;

layout(location=0) out vec3 vs_outPosition;
layout(location=1) out vec3 vs_outNormal;
layout(location=2) out vec2 vs_outUV;

void main()
{
    gl_Position = shaderData.matProjection * shaderData.matView * shaderData.matModel * vec4(in_Position, 1.0f);

    vs_outPosition = (shaderData.matModel * vec4(in_Position, 1.0f)).xyz;
    vs_outNormal = normalize(shaderData.matModel * vec4(in_Normal, 0.0f)).xyz;
    vs_outUV = in_UV;
}