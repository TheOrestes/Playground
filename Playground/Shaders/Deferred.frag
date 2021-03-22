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
    int  passID;
    vec3 cameraPosition;
} shaderData;

// Final color output!
layout(location = 0) out vec4 outColor;

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

    // Remap Depth!
    float lowerBound = 0.98f;
    float upperBound = 1.0f;
    float ScaledDepth = 1.0f - ((Depth-lowerBound)/(upperBound-lowerBound));

    //--- 3 Side Direct Lighting
    vec3 lightDir[3] = { vec3(0.70711, 0.24185, 0.66447), 
                         vec3(-0.70711, 0.24185, 0.66447), 
                         vec3(-0.70711, 0.24185, -0.66447)};

    float lightIntensity[3] = { 5,5,5};

    vec3 Half   = vec3(0);
    vec4 Lo     = vec4(0);
    float Kd    = 0.8f;
    float Ks    = 1 - Kd;

    for(int i = 0 ; i < 3 ; ++i)
    {
        vec3 N = normalize(NormalColor.xyz);
        float diffuse = lightIntensity[i] * clamp(dot(N, lightDir[i]), 0.0f, 1.0f);

        vec3 cameraPos = shaderData.cameraPosition;
        vec3 Eye = normalize(cameraPos - PositionColor.rgb);
        Half = normalize(lightDir[i] + Eye);    

        Lo += (Kd * AlbedoColor * PI_INVERSE) * diffuse;
    }
    
    // Composite BackgroundColor (Blue channel of ObjectID) + Final Color 
    vec4 FinalColor = vec4(0); 
    vec3 redChannel = vec3(1,0,0);  // STATIC_OPAQUE
    vec3 blueChannel = vec3(0,0,1); // SKYBOX

    if(dot(redChannel, ObjectIDColor.rgb) == 1)
        FinalColor = Lo;
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