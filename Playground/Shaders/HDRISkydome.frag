#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input from Vertex Shader
layout(location = 0) in vec4 vs_outPosition;
layout(location = 1) in vec3 vs_outNormal;
layout(location = 2) in vec2 vs_outUV;

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4 matModel;
    mat4 matView;
    mat4 matProjection;
} shaderData;

// Uniform variable
layout(set = 0, binding = 1) uniform sampler2D   samplerHDRI;

// output to second subpass!
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;        // RGB - Normal | A- unused
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec4 outPBR;           // R - Metalness | G - Roughness | B - AO | A - Unused
layout(location = 4) out vec4 outEmission;      // RGB - Emission / A - Unused
layout(location = 5) out vec4 outBackground;    // RGB - BG Color / A - Unused
layout(location = 6) out vec4 outObjID;         // RGB - ID Color / A - Unused

void main() 
{
    // Sampler Input Textures!
    vec4 hdriColor  = texture(samplerHDRI, vs_outUV);

    // Write to Position G-Buffer
    //outPosition = vs_outPosition;

    //**** TODO - Write to Background G-Buffer
    outBackground = hdriColor;

    outObjID = vec4(0, 0, 1, 1);
     // Write to ID buffer
     // switch(shaderData.objectID)
     // {
     //     case 0: // SKYBOX
     //     {
     //         outObjID = vec4(0, 0, 1, 1);
     //         break;
     //     }
     // }
}