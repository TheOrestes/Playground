#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location=0) in vec3 in_Position;
layout(location=1) in vec3 in_Normal;
layout(location=2) in vec2 in_Texcoord;

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 matModel;
    mat4 matView;
    mat4 matProjection;
} shaderData;

layout(set = 0, binding = 1) uniform sampler2D   samplerHDRI;

layout(location=0) out vec4 vs_outPosition;
layout(location=1) out vec3 vs_outNormal;
layout(location=2) out vec2 vs_outUV;

void main()
{
    // Remove the translation part from the view matrix!
    mat4 rotView = mat4(mat3(shaderData.matView));

    vec4 Pos = shaderData.matProjection * rotView * vec4(in_Position, 1.0f);

    // Idea is to make Z component always 1 for maximum depth!
    gl_Position = Pos.xyww;

    //vs_outPosition  = Pos.xyww;
    //vs_outNormal = (shaderData.matModel * vec4(in_Normal, 0)).xyz;
    vs_outUV = in_Texcoord;
}