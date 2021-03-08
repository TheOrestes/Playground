#version 450

// Input from Subpass 1
layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;    // Color output from Subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;    // Depth output from the subpass 1
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inputNormal;   // Normal output from the subpass 1
layout(input_attachment_index = 3, binding = 3) uniform subpassInput inputPosition; // Position output from the subpass 1

// Final color output!
layout(location = 0) out vec4 outColor;

void main()
{
    // extract subpass 1 g-buffer information
    vec4 albedoColor = subpassLoad(inputColor).rgba;
    float depth = subpassLoad(inputDepth).r;

    float lowerBound = 0.98f;
    float upperBound = 1.0f;
    float scaledDepth = 1.0f - ((depth-lowerBound)/(upperBound-lowerBound));

    vec4 normalColor = subpassLoad(inputNormal).rgba;
    vec4 positionColor = subpassLoad(inputPosition).rgba;

    //--- Lighting
    // Assumed light position
    vec3 lightPos = vec3(0,3,3);
    vec3 lightDir = normalize(positionColor.xyz - lightPos);
    float lightDist = length(positionColor.xyz - lightPos);

    float attenuation = 1 / 1;

    vec3 N = normalize(normalColor.xyz);
    float diffuse = attenuation * clamp(dot(N, -lightDir), 0.0f, 1.0f);

    //outColor = vec4(vec3(scaledDepth), 1.0f);
    //outColor = normalColor;
    //outColor = positionColor;
    outColor = vec4(vec3(diffuse), 1.0f) * albedoColor;
}