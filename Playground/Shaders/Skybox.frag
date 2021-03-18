#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input from Vertex Shader
layout(location = 0) in vec4 vs_outPosition;
layout(location = 1) in vec3 vs_outUVW;

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 matModel;
    mat4 matView;
    mat4 matProjection;
    int  objectID;
} shaderData;

// Uniform variable
layout(set = 0, binding = 1) uniform samplerCube samplerCubemapTexture;

// output to second subpass!
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;        // RGB - Normal | A- unused
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec4 outPBR;           // R - Metalness | G - Roughness | B - AO | A - Unused
layout(location = 4) out vec4 outEmission;      // RGB - Emission / A - Unused
layout(location = 5) out vec4 outBackground;    // RGB - BG Color / A - Unused

void main() 
{
    // Sampler Input Textures!
    vec4 CubemapColor  = texture(samplerCubemapTexture, vs_outUVW);

    // Write to Position G-Buffer
    outPosition = vs_outPosition;

    //**** TODO - Write to Background G-Buffer
    outBackground = CubemapColor;
}