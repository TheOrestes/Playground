#version 450

layout(input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;    // Color output from Subpass 1
layout(input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;    // Depth output from the subpass 1
layout(input_attachment_index = 2, binding = 2) uniform subpassInput inputNormal;   // Normal output from the subpass 1

layout(location = 0) out vec4 outColor;

void main()
{
    vec4 screenColor = subpassLoad(inputColor).rgba;

    float depth = subpassLoad(inputDepth).r;
    float lowerBound = 0.95f;
    float upperBound = 1.0f;

    float scaledDepth = 1.0f - ((depth-lowerBound)/(upperBound-lowerBound));

    vec4 normalColor = subpassLoad(inputNormal).rgba;

    //outColor = vec4(vec3(scaledDepth), 1.0f);
    outColor = normalColor;
}