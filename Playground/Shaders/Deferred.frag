#version 450

#define PI 3.14159265358979
#define PI_OVER_TWO 1.57079632679
#define PI_INVERSE 0.3183098861837

// Input from Subpass 1
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;        // Color output from Subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;        // Depth output from the subpass 1
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inputNormal;       // Normal output from the subpass 1
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inputPosition;     // Position output from the subpass 1
layout(input_attachment_index = 4, binding = 4) uniform subpassInput inputPBR;          // PBR output from the subpass 1
layout(input_attachment_index = 5, binding = 5) uniform subpassInput inputEmission;     // Emission output from the subpass 1
layout(input_attachment_index = 6, binding = 6) uniform subpassInput inputBackground;   // Background output from the subpass 1
layout(input_attachment_index = 7, binding = 7) uniform subpassInput inputObjectID;     // ObjectID output from the subpass 1

layout(set = 0, binding = 8) uniform DeferredShaderData
{
    vec4 lightProperties;   // RGB - Direction, A - Intensity
    vec3 cameraPosition;
    int  passID;
} shaderData;

// Final color output!
layout(location = 0) out vec4 outColor;

//---------------------------------------------------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    
    float num    = a2;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    denom        = PI * denom * denom;
    
    return num / denom;
}

//---------------------------------------------------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r     = (roughness + 1.0);
    float k     = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

//---------------------------------------------------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

//---------------------------------------------------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}  

//---------------------------------------------------------------------------------------------------------------------
void main()
{
    // extract subpass-1 G-Buffer information
    vec4 AlbedoColor    = subpassLoad(inputColor).rgba;
    float Depth         = subpassLoad(inputDepth).r;
    vec4 NormalColor    = subpassLoad(inputNormal).rgba;
    vec4 PositionColor  = subpassLoad(inputPosition).rgba; 
    vec4 PBRColor       = subpassLoad(inputPBR).rgba; 
    vec4 EmissionColor  = subpassLoad(inputEmission).rgba;
    vec4 BackgroundColor= subpassLoad(inputBackground).rgba;
    vec4 ObjectIDColor  = subpassLoad(inputObjectID).rgba;

    float Metalness     = PBRColor.r;
    float Roughness     = PBRColor.g;
    float Occlusion     = PBRColor.b;

    // Remap Depth!
    float lowerBound = 0.98f;
    float upperBound = 1.0f;
    float ScaledDepth = 1.0f - ((Depth-lowerBound)/(upperBound-lowerBound));


    //-- Shading calculations!
    vec3 Half   = vec3(0);
    vec3 Lo     = vec3(0);
 
    vec3 N      = normalize(NormalColor.xyz);
    vec3 Eye    = normalize(shaderData.cameraPosition - PositionColor.rgb);

    vec3 LightDir = normalize(shaderData.lightProperties.rgb);
    float LightIntensity = shaderData.lightProperties.a;

    vec3 F0     = vec3(0.04f);
    F0          = mix(F0, AlbedoColor.rgb, Metalness);

    Half = normalize(LightDir + Eye);    

    float NdotL = max(dot(N, LightDir), 0.0f);
    float HdotV = max(dot(Half, Eye), 0.0f);
    float NdotV = max(dot(N, Eye), 0.0f);
    
    // Cook-Torrance BRDF
    float D = DistributionGGX(N, Half, Roughness);
    float G = GeometrySmith(N, Eye, LightDir, Roughness);
    vec3  F = fresnelSchlick(HdotV, F0);

    vec3 Ks = F;
    vec3 Kd = vec3(1) - Ks;
    Kd     *= 1.0f - Metalness;

    vec3 Nr = D * G * F;
    float Dr = 4.0f * max(NdotV, 0.0f) * NdotL;

    vec3 Specular = Nr / max(Dr, 0.001f);

    Lo += vec3(NdotL); //(Kd * AlbedoColor.rgb * PI_INVERSE + Specular) * LightIntensity * NdotL;

    vec3 Ambient = AlbedoColor.rgb * Occlusion;
    vec3 Color = /*Ambient + */Lo;

    //Color = Color / (Color + vec3(1));
    //Color = pow(Color, vec3(1.0f/2.2f));
    
    // Composite BackgroundColor (Blue channel of ObjectID) + Final Color 
    vec4 FinalColor = vec4(0); 
    vec3 redChannel = vec3(1,0,0);  // STATIC_OPAQUE
    vec3 blueChannel = vec3(0,0,1); // SKYBOX

    if(dot(redChannel, ObjectIDColor.rgb) == 1)
        FinalColor = vec4(Color, 1);
    else if(dot(blueChannel, ObjectIDColor.rgb) == 1)
        FinalColor = BackgroundColor;
    
    // DEBUG: Individual Passes!
    switch(shaderData.passID)
    {
        case 0:
        {
            outColor = FinalColor;
        }   break;

        case 1:
        {
            outColor = AlbedoColor;                                 
        }   break;

        case 2:
        {
            outColor = vec4(vec3(ScaledDepth), 1);                  
        }   break;

        case 3:
        {
            outColor = PositionColor;                               
        }   break;

        case 4:
        {
            outColor = NormalColor;                                 
        }   break;

        case 5:
        {
            outColor = vec4(vec3(PBRColor.r), 1);                   
        }   break;

        case 6:
        {
            outColor = vec4(vec3(PBRColor.g), 1);                   
        }   break;

        case 7:
        {
            outColor = vec4(vec3(PBRColor.b), 1);                   
        }   break;

        case 8:
        {
            outColor = EmissionColor;                               
        }   break;

        case 9:
        {
            outColor = BackgroundColor;                             
        }   break;

        case 10:
        {
            outColor = ObjectIDColor;                             
        }   break;
    }
    
   // outColor = albedoColor + Kd * vec4(vec3(diffuse), 1) + Ks * vec4(vec3(specular), 1.0f);
}