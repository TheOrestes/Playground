#include "PlaygroundPCH.h"
#include "VulkanGraphicsPipeline.h"

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

#include "Engine/Helpers/Utility.h"
#include "Engine/Helpers/Log.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

//---------------------------------------------------------------------------------------------------------------------
VulkanGraphicsPipeline::VulkanGraphicsPipeline(PipelineType type, VulkanSwapChain* pSwapChain)
{
	m_vkPipelineLayout = VK_NULL_HANDLE;
	m_vkGraphicsPipeline = VK_NULL_HANDLE;

	m_strVertexShader.clear();
	m_strFragmentShader.clear();

	m_eType = type;

	//--- Input assembly 
	m_vkInputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	m_vkInputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;						// primitive type to single list
	m_vkInputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	m_vkInputAssemblyInfo.flags = 0;
	m_vkInputAssemblyInfo.pNext = nullptr;

	//--- Viewport rect
	m_vkViewportInfo.x = 0.0f;
	m_vkViewportInfo.y = 0.0f;
	m_vkViewportInfo.width = (float)pSwapChain->m_vkSwapchainExtent.width;
	m_vkViewportInfo.height = (float)pSwapChain->m_vkSwapchainExtent.height;
	m_vkViewportInfo.minDepth = 0.0f;
	m_vkViewportInfo.maxDepth = 1.0f;

	//--- Scissor rect 
	m_vkScissorRect.offset = { 0, 0 };															// offset to use region from
	m_vkScissorRect.extent = pSwapChain->m_vkSwapchainExtent;									// extent to describe region to use, starting at offset!

	//--- combine viewport & scissor rect info to create viewport info!
	m_vkViewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	m_vkViewportStateInfo.flags = 0;
	m_vkViewportStateInfo.pNext = nullptr;
	m_vkViewportStateInfo.pScissors = &m_vkScissorRect;
	m_vkViewportStateInfo.pViewports = &m_vkViewportInfo;
	m_vkViewportStateInfo.scissorCount = 1;
	m_vkViewportStateInfo.viewportCount = 1;

	//--- Rasterizer
	m_vkRasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	m_vkRasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;									// which face of a triangle to cull
	m_vkRasterizerCreateInfo.depthClampEnable = VK_FALSE;										// change if fragments beyond near/far planes are clipped.
	m_vkRasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;						// treat clockwise as front side
	m_vkRasterizerCreateInfo.lineWidth = 1.0f;													// how thick the line should be drawn
	m_vkRasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;								// how to handle filling points between vertices
	m_vkRasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;								// whether to discard data & skip rasterizer. Never creates fragments, only suitable for pipelines without framebuffers!
	m_vkRasterizerCreateInfo.depthBiasEnable = VK_FALSE;										// whether to add depth bias to fragments (good for stopping "shadow acne")
	m_vkRasterizerCreateInfo.depthBiasClamp = 0.0f;
	m_vkRasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
	m_vkRasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;
	m_vkRasterizerCreateInfo.flags = 0;
	m_vkRasterizerCreateInfo.pNext = nullptr;

	//--- Multi-sampling
	m_vkMultisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	m_vkMultisamplingCreateInfo.sampleShadingEnable = VK_FALSE;									// Enable multisampling or not
	m_vkMultisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;					// number of samples per fragment
	m_vkMultisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
	m_vkMultisamplingCreateInfo.alphaToOneEnable = VK_FALSE;
	m_vkMultisamplingCreateInfo.flags = 0;
	m_vkMultisamplingCreateInfo.minSampleShading = 1.0f;
	m_vkMultisamplingCreateInfo.pNext = nullptr;
	m_vkMultisamplingCreateInfo.pSampleMask = nullptr;


	//--- Depth & Stencil testing 
	m_vkDepthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	m_vkDepthStencilCreateInfo.depthTestEnable = VK_FALSE;										// Enable depth check to determine fragment write
	m_vkDepthStencilCreateInfo.depthWriteEnable = VK_FALSE;										// Enable writing to depth buffer to replace old values
	m_vkDepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;					// Comparison opearation that allows an overwrite (is in front)
	m_vkDepthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;								// Depth bounds test, does the depth value exist between two bounds!
	m_vkDepthStencilCreateInfo.stencilTestEnable = VK_FALSE;									// Enable/Disable Stencil test

	m_vkVertexInputStateCreateInfo = {};
}

//---------------------------------------------------------------------------------------------------------------------
VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CreatePipelineLayout(VulkanDevice* pDevice, const std::vector<VkDescriptorSetLayout>& layouts,
													VkPushConstantRange pushConstantRange)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = layouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	// create pipeline layout
	if (vkCreatePipelineLayout(pDevice->m_vkLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Pipeline layout!");
	}
	else
		LOG_INFO("Created Pipeline layout");
}

//---------------------------------------------------------------------------------------------------------------------
VkShaderModule VulkanGraphicsPipeline::CreateShaderModule(VulkanDevice* pDevice, const std::string& fileName)
{
	// start reading at the end & in binary mode.
	// Advantage of reading file from the end is we can use read position to determine
	// size of the file & allocate buffer accordingly!
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		LOG_ERROR("Failed to open Shader file!");

	// get the file size & allocate buffer memory!
	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	// now seek back to the beginning of the file & read all bytes at once!
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	// close the file!
	file.close();

	// Create Shader Module
	VkShaderModuleCreateInfo shaderModuleInfo;
	shaderModuleInfo.codeSize = buffer.size();
	shaderModuleInfo.flags = 0;
	shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());
	shaderModuleInfo.pNext = nullptr;
	shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(pDevice->m_vkLogicalDevice, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS)
		LOG_ERROR("Failed to create shader module!");

	return shaderModule;
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CreateGraphicsPipeline(VulkanDevice* pDevice, VulkanSwapChain* pSwapChain, 
													VkRenderPass renderPass, uint32_t subPass, uint32_t nOutputAttachments)
{
	VkShaderModule vertShaderModule = VK_NULL_HANDLE;
	VkShaderModule fragShaderModule = VK_NULL_HANDLE;

	switch (m_eType)
	{
		case PipelineType::GBUFFER_OPAQUE:
			{
				m_strVertexShader = "Shaders/GBuffer.vert.spv";
				m_strFragmentShader = "Shaders/GBuffer.frag.spv";

				vertShaderModule = CreateShaderModule(pDevice, m_strVertexShader);
				fragShaderModule = CreateShaderModule(pDevice, m_strFragmentShader);

				//--- How the data for the single vertex (including info such as Position, color, texcoords etc.) is as a whole
				VkVertexInputBindingDescription bindingDescription = {};
				bindingDescription.binding = 0; // can bind multiple stream of data, this defines which one?
				bindingDescription.stride = sizeof(Helper::App::VertexPNTBT); // size of single vertex object
				bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
				// How to move between data after each vertex
				// VK_VERTEX_INPUT_RATE_VERTEX : move on to the next vertex																							// VK_VERTEX_INPUT_RATE_INSTANCE: move on to a vertex of next instance.
				// How the data for an attribute is defined within a vertex
				std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions;

				// Position attribute
				attributeDescriptions[0].binding = 0; // which binding the data is at (should be same as above)
				attributeDescriptions[0].location = 0; // location in shader where data will be read from
				attributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				// format the data will take (also helps define size of the data)
				attributeDescriptions[0].offset = offsetof(Helper::App::VertexPNTBT, Position);
				// where this attribute is defined in the data for a single vertex

				// Normal attribute
				attributeDescriptions[1].binding = 0;
				attributeDescriptions[1].location = 1;
				attributeDescriptions[1].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[1].offset = offsetof(Helper::App::VertexPNTBT, Normal);

				// Tangent attribute
				attributeDescriptions[2].binding = 0;
				attributeDescriptions[2].location = 2;
				attributeDescriptions[2].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[2].offset = offsetof(Helper::App::VertexPNTBT, Tangent);

				// BiNormal attribute
				attributeDescriptions[3].binding = 0;
				attributeDescriptions[3].location = 3;
				attributeDescriptions[3].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
				attributeDescriptions[3].offset = offsetof(Helper::App::VertexPNTBT, BiNormal);

				// Texture attribute
				attributeDescriptions[4].binding = 0;
				attributeDescriptions[4].location = 4;
				attributeDescriptions[4].format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
				attributeDescriptions[4].offset = offsetof(Helper::App::VertexPNTBT, UV);

				// Vertex Input
				m_vkVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				m_vkVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
				m_vkVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
				// List of vertex attribute descriptions (data format & where to bind to - from)
				m_vkVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
				m_vkVertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
				// List of vertex binding descriptions (data spacing/strides info) 
				m_vkVertexInputStateCreateInfo.flags = 0;
				m_vkVertexInputStateCreateInfo.pNext = nullptr;

				// Enable depth testing during normal rendering!
				m_vkDepthStencilCreateInfo.depthTestEnable = VK_TRUE;						// Enable depth check to determine fragment write
				m_vkDepthStencilCreateInfo.depthWriteEnable = VK_TRUE;						// Enable writing to depth buffer to replace old values
				m_vkDepthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;				// Comparison opearation that allows an overwrite (is in front)c vb

				//--- Color blending
				// We need to explicitly mention the blending setting between all output attachments else default colormask will be 0x0
				// and nothing will be rendered to the attachment!bv  
				m_vecColorBlendAttachments.clear();
				for (int i = 0; i < nOutputAttachments; ++i)
				{
					VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
					colorBlendAttachment.colorWriteMask = 0xf; // All channels
					colorBlendAttachment.blendEnable = VK_FALSE;

					m_vecColorBlendAttachments.push_back(colorBlendAttachment);
				}

				m_vkColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				m_vkColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
				// alternative to calculations is to use logical operations
				m_vkColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
				m_vkColorBlendStateCreateInfo.attachmentCount = m_vecColorBlendAttachments.size();
				m_vkColorBlendStateCreateInfo.pAttachments = m_vecColorBlendAttachments.data();
				m_vkColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
				m_vkColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
				m_vkColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
				m_vkColorBlendStateCreateInfo.blendConstants[3] = 0.0f;
				m_vkColorBlendStateCreateInfo.flags = 0;
				m_vkColorBlendStateCreateInfo.pNext = nullptr;


				break;
			}
		

		case PipelineType::SKYBOX:
		{
			m_strVertexShader = "Shaders/Skybox.vert.spv";
			m_strFragmentShader = "Shaders/Skybox.frag.spv";

			vertShaderModule = CreateShaderModule(pDevice, m_strVertexShader);
			fragShaderModule = CreateShaderModule(pDevice, m_strFragmentShader);

			//--- How the data for the single vertex (including info such as Position, color, texcoords etc.) is as a whole
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;															// can bind multiple stream of data, this defines which one?
			bindingDescription.stride = sizeof(Helper::App::VertexP);								// size of single vertex object
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;								// How to move between data after each vertex
																									// VK_VERTEX_INPUT_RATE_VERTEX : move on to the next vertex																							// VK_VERTEX_INPUT_RATE_INSTANCE: move on to a vertex of next instance.
			// How the data for an attribute is defined within a vertex
			std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions;

			// Position attribute
			attributeDescriptions[0].binding = 0;													// which binding the data is at (should be same as above)
			attributeDescriptions[0].location = 0;													// location in shader where data will be read from
			attributeDescriptions[0].format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;					// format the data will take (also helps define size of the data)
			attributeDescriptions[0].offset = offsetof(Helper::App::VertexP, Position);				// where this attribute is defined in the data for a single vertex

			// Vertex Input
			m_vkVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			m_vkVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			m_vkVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();			
			m_vkVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			m_vkVertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;						
			m_vkVertexInputStateCreateInfo.flags = 0;
			m_vkVertexInputStateCreateInfo.pNext = nullptr;

			//--- Color blending
			// We need to explicitly mention the blending setting between all output attachments else default colormask will be 0x0
			// and nothing will be rendered to the attachment!
			m_vecColorBlendAttachments.clear();
			for (int i = 0; i < nOutputAttachments; ++i)
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = 0xf;							// All channels
				colorBlendAttachment.blendEnable = VK_FALSE;

				m_vecColorBlendAttachments.push_back(colorBlendAttachment);
			}

			m_vkColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			m_vkColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;										// alternative to calculations is to use logical operations
			m_vkColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
			m_vkColorBlendStateCreateInfo.attachmentCount = m_vecColorBlendAttachments.size();
			m_vkColorBlendStateCreateInfo.pAttachments = m_vecColorBlendAttachments.data();
			m_vkColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
			m_vkColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
			m_vkColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
			m_vkColorBlendStateCreateInfo.blendConstants[3] = 0.0f;
			m_vkColorBlendStateCreateInfo.flags = 0;
			m_vkColorBlendStateCreateInfo.pNext = nullptr;

			break;
		}
			
		case PipelineType::DEFERRED:
		{
			m_strVertexShader = "Shaders/Deferred.vert.spv";
			m_strFragmentShader = "Shaders/Deferred.frag.spv";

			vertShaderModule = CreateShaderModule(pDevice, m_strVertexShader);
			fragShaderModule = CreateShaderModule(pDevice, m_strFragmentShader);

			// No vertex data for Final beauty 
			m_vkVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			m_vkVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
			m_vkVertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
			m_vkVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
			m_vkVertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;

			// disable writing to depth buffer
			m_vkDepthStencilCreateInfo.depthWriteEnable = VK_FALSE;

			//--- Color blending
			// We need to explicitly mention the blending setting between all output attachments else default colormask will be 0x0
			// and nothing will be rendered to the attachment!
			m_vecColorBlendAttachments.clear();
			for (int i = 0; i < nOutputAttachments; ++i)
			{
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = 0xf;							// All channels
				colorBlendAttachment.blendEnable = VK_FALSE;

				m_vecColorBlendAttachments.push_back(colorBlendAttachment);
			}

			m_vkColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			m_vkColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;										// alternative to calculations is to use logical operations
			m_vkColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
			m_vkColorBlendStateCreateInfo.attachmentCount = m_vecColorBlendAttachments.size();
			m_vkColorBlendStateCreateInfo.pAttachments = m_vecColorBlendAttachments.data();
			m_vkColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
			m_vkColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
			m_vkColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
			m_vkColorBlendStateCreateInfo.blendConstants[3] = 0.0f;
			m_vkColorBlendStateCreateInfo.flags = 0;
			m_vkColorBlendStateCreateInfo.pNext = nullptr;

			break;
		}
	}
	
	//--- to actually use shaders, we need to assign them to a specific pipeline stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


	// Dynamic state (nullptr for now!)

	// Finally, Create Graphics Pipeline!!!
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &m_vkVertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &m_vkInputAssemblyInfo;
	graphicsPipelineCreateInfo.pViewportState = &m_vkViewportStateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.pRasterizationState = &m_vkRasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &m_vkMultisamplingCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &m_vkColorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &m_vkDepthStencilCreateInfo;
	graphicsPipelineCreateInfo.layout = m_vkPipelineLayout;
	graphicsPipelineCreateInfo.renderPass = renderPass;
	graphicsPipelineCreateInfo.subpass = subPass;
	graphicsPipelineCreateInfo.pNext = nullptr;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.flags = 0;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;


	if (vkCreateGraphicsPipelines(pDevice->m_vkLogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
	{
		LOG_ERROR("Failed to create Graphics Pipeline!");
	}
	else
		LOG_INFO("Created Graphics Pipeline!");

	// Destroy shader modules...
	vkDestroyShaderModule(pDevice->m_vkLogicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(pDevice->m_vkLogicalDevice, fragShaderModule, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::Cleanup(VulkanDevice* pDevice)
{
	vkDestroyPipeline(pDevice->m_vkLogicalDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(pDevice->m_vkLogicalDevice, m_vkPipelineLayout, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VulkanGraphicsPipeline::CleanupOnWindowResize(VulkanDevice* pDevice)
{
	vkDestroyPipeline(pDevice->m_vkLogicalDevice, m_vkGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(pDevice->m_vkLogicalDevice, m_vkPipelineLayout, nullptr);
}


