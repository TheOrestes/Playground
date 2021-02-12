#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vs_outPosition;
layout(location = 1) in vec3 vs_outNormal;
layout(location = 2) in vec2 vs_outUV;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

void main() 
{
    outColor = texture(textureSampler, vs_outUV);
    outNormal = vec4(vs_outNormal, 1.0f);
}