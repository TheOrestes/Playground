#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 in_Position;

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 matModel;
    mat4 matView;
    mat4 matProjection;
    int  objectID;
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

layout(location=0) out vec4 vs_outPosition;
layout(location=1) out vec3 vs_outUVW;

void main()
{
    vec4 Pos = shaderData.matProjection * shaderData.matView * shaderData.matModel * vec4(in_Position, 1.0f);

    // Idea is to make Z component always 1 for maximum depth!
    gl_Position = Pos.xyww;

    vs_outPosition  = Pos.xyww;
    vs_outUVW = in_Position;
}