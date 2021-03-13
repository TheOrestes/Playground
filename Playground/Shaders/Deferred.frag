#version 450

// Input from Subpass 1
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;        // Color output from Subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;        // Depth output from the subpass 1
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inputNormal;       // Normal output from the subpass 1
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inputPosition;     // Position output from the subpass 1
layout(input_attachment_index = 4, binding = 4) uniform subpassInput inputPBR;          // PBR output from the subpass 1
layout(input_attachment_index = 5, binding = 5) uniform subpassInput inputEmission;     // Emission output from the subpass 1
layout(input_attachment_index = 6, binding = 6) uniform subpassInput inputBackground;   // Background output from the subpass 1

layout(set = 0, binding = 7) uniform DeferredShaderData
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

    // Remap Depth!
    float lowerBound = 0.98f;
    float upperBound = 1.0f;
    float ScaledDepth = 1.0f - ((Depth-lowerBound)/(upperBound-lowerBound));

    //--- Lighting
    // Assumed light position
    //vec3 lightPos = vec3(0,3,3);
    //vec3 lightDir = normalize(positionColor.xyz - lightPos);
    vec3 lightDir = vec3(0,1,0);
    float lightDist = length(lightDir);

    float attenuation = 1 / 1;

    vec3 N = normalize(NormalColor.xyz);
    float diffuse = attenuation * clamp(dot(N, lightDir), 0.0f, 1.0f);

    vec3 cameraPos = shaderData.cameraPosition;
    vec3 Eye = normalize(cameraPos - PositionColor.rgb);
    vec3 Half = normalize(lightDir + Eye);

    float Kd = 0.8f;
    float Ks = 1 - Kd;
    
    // FIXME: There has to be a better way than this!!
    switch(shaderData.passID)
    {
        case 0:
        {
            outColor = AlbedoColor + Kd * vec4(vec3(diffuse), 1);   
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
    }
    
   // outColor = albedoColor + Kd * vec4(vec3(diffuse), 1) + Ks * vec4(vec3(specular), 1.0f);
}