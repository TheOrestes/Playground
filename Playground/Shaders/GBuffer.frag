#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input from Vertex Shader
layout(location = 0) in vec3 vs_outPosition;
layout(location = 1) in vec3 vs_outNormal;
layout(location = 2) in vec3 vs_outTangent;
layout(location = 3) in vec3 vs_outBiNormal;
layout(location = 4) in vec2 vs_outUV;

// Uniform variable
layout(set = 0, binding = 1) uniform sampler2D samplerAlbedo;
layout(set = 0, binding = 2) uniform sampler2D samplerSpecular;
layout(set = 0, binding = 3) uniform sampler2D samplerNormal;

// output to second subpass!
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outPosition;

void main() 
{
    outColor = texture(samplerAlbedo, vs_outUV);

    // Calculate normal in Tangent space
    vec3 N = normalize(vs_outNormal);
    vec3 T = normalize(vs_outTangent);
    vec3 B = normalize(cross(N,T));

    mat3 TBN = mat3(T, B, N);
    vec3 tNormal = TBN * normalize(texture(samplerNormal, vs_outUV).xyz * 2.0f - vec3(1.0f));

    outNormal = vec4(tNormal, 1.0f);
    outPosition = vec4(vs_outPosition, 1.0f);
}