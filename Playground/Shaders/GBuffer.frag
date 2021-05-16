#version 450
#extension GL_ARB_separate_shader_objects : enable

// Input from Vertex Shader
layout(location = 0) in vec3 vs_outPosition;
layout(location = 1) in vec3 vs_outNormal;
layout(location = 2) in vec3 vs_outTangent;
layout(location = 3) in vec3 vs_outBiNormal;
layout(location = 4) in vec2 vs_outUV;

layout(set = 0, binding = 0) uniform ShaderData
{
    mat4    matModel;
    mat4    matView;
    mat4    matProjection;

    vec4    albedoColor;
    vec4    emissiveColor;
    vec3    hasTextureAEN;
    vec3    hasTextureRMO;

    float   ao;
    float   roughness;
    float   metalness;
    int     objectID;
} shaderData;

// Uniform variable
layout(set = 0, binding = 1) uniform sampler2D   samplerBaseTexture;
layout(set = 0, binding = 2) uniform sampler2D   samplerMetalnessTexture;
layout(set = 0, binding = 3) uniform sampler2D   samplerNormalTexture;
layout(set = 0, binding = 4) uniform sampler2D   samplerRoughnessTexture;
layout(set = 0, binding = 5) uniform sampler2D   samplerAOTexture;
layout(set = 0, binding = 6) uniform sampler2D   samplerEmissionTexture;

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
    vec4 baseColor          = vec4(0.0f);
    vec4 MetalnessColor     = vec4(0.0f);
    vec4 NormalColor        = vec4(0.0f);
    vec4 RoughnessColor     = vec4(0.0f);
    vec4 AOColor            = vec4(0.0f);
    vec4 EmissionColor      = vec4(0.0f);
    vec4 IrradianceColor    = vec4(0.0f);
    vec4 PrefilterSpecColor = vec4(0.0f);

    //---- Extract Base Color
    if(shaderData.hasTextureAEN.r == 1)
        baseColor       = texture(samplerBaseTexture, vs_outUV);
    else    
        baseColor       = shaderData.albedoColor;

    //---- Extract Emissive Color
    if(shaderData.hasTextureAEN.g == 1)
        EmissionColor   = texture(samplerEmissionTexture, vs_outUV);
    else
        EmissionColor   = shaderData.emissiveColor;

    //---- Extract Normal Color
    vec3 Normal = vec3(0);
    if(shaderData.hasTextureAEN.b == 1)
    {
        NormalColor = texture(samplerNormalTexture, vs_outUV);
          
        // Calculate normal in Tangent space
        vec3 N = normalize(vs_outNormal);
        vec3 T = normalize(vs_outTangent);
        vec3 B = normalize(cross(N,T));

        mat3 TBN = mat3(T, B, N);
        Normal = TBN * normalize(NormalColor.rgb * 2.0f - vec3(1.0f));
    }
    else
        Normal = normalize(((vs_outNormal) + vec3(1)) / 2.0f);

    //---- Extract Roughness Color
    if(shaderData.hasTextureRMO.r == 1)
        RoughnessColor  = texture(samplerRoughnessTexture, vs_outUV);
    else    
        RoughnessColor  = vec4(vec3(shaderData.roughness), 1);

    //---- Extract Metalness Color
    if(shaderData.hasTextureRMO.g == 1)
        MetalnessColor  = texture(samplerMetalnessTexture, vs_outUV);
    else    
        MetalnessColor  = vec4(vec3(shaderData.metalness), 1);

    //---- Extract Occlusion Color
    if(shaderData.hasTextureRMO.b == 1)
        AOColor         = texture(samplerAOTexture, vs_outUV);
    else    
        AOColor         = vec4(vec3(shaderData.ao), 1);    

     // Write to Color G-Buffer
    outColor = baseColor;
 
    // Write to Normal G-Buffer
    outNormal = vec4(Normal, 0.0f);

    // Write to Position G-Buffer
    outPosition = vec4(vs_outPosition, 1.0f);

    // Write to PBR G-Buffer
    outPBR = vec4(MetalnessColor.r, RoughnessColor.r, AOColor.r, 0.0f);

    // Write to Emission G-Buffer
    outEmission = vec4(EmissionColor.rgb, 0.0f);

    // Write to ID buffer
    switch(shaderData.objectID)
    {
        case 1: // STATIC_OPAQUE
        {
            outObjID = vec4(1, 0, 0, 1);
            break;
        }
    }
    
}