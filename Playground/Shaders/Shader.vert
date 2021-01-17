#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 in_Position;
layout(location=1) in vec3 in_Color;
layout(location=2) in vec2 in_UV;

layout(set = 0, binding = 0) uniform UboViewProjection
{
    mat4 matProjection;
    mat4 matView;
} uboViewProj;

// NOT IN USE, LEFT FOR REFERENCE
// layout(set = 0, binding = 1) uniform UboModel
// {
//     mat4 matModel;
// } uboModel;

layout (push_constant) uniform PushModel
{
    mat4 matModel;
} pushModel;

layout(location=0) out vec3 vs_outColor;
layout(location=1) out vec2 vs_outUV;

void main()
{
    gl_Position = uboViewProj.matProjection * uboViewProj.matView * pushModel.matModel * vec4(in_Position, 1.0f);

    vs_outColor = in_Color;
    vs_outUV = in_UV;
}