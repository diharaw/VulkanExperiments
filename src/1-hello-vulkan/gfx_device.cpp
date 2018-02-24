#include "gfx_device.h"
#include <assert.h>
#include <iostream>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : " << __FILE__ << " at line " << __LINE__ << std::endl;					\
		assert(res == VK_SUCCESS);																		\
	}																									\
}

namespace gfx
{
	const VkFormat g_InputAttribFormatTable[][4] =
	{
		{ VK_FORMAT_R8_SINT, VK_FORMAT_R8G8_SINT, VK_FORMAT_R8G8B8_SINT, VK_FORMAT_R8G8B8A8_SINT },
		{ VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R8G8B8A8_UINT },
		{ VK_FORMAT_R16_SINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R16G16B16A16_SINT },
		{ VK_FORMAT_R32_SINT, VK_FORMAT_R32G32_SINT, VK_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32A32_SINT },
		{ VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R16G16B16A16_UINT },
		{ VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32A32_UINT },
		{ VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT },
	};

	bool Device::Init()
	{
		return 0;
	}

	//InputElement elements[] =
	//{
	//	{ 3, DataType::FLOAT, false, 0, "POSITION" },
	//{ 3, DataType::FLOAT, false, sizeof(float) * 3, "NORMAL" }
	////        { 2, DataType::FLOAT, false, sizeof(float) * 3, "TEXCOORD" }
	//};

	//InputLayoutCreateDesc ilcd;
	//DW_ZERO_MEMORY(ilcd);
	//ilcd.elements = elements;
	//ilcd.num_elements = 2;
	//ilcd.vertex_size = sizeof(float) * 6;

	InputLayout* Device::CreateInputLayout(const InputLayoutCreateDesc& desc)
	{
		InputLayout* il = new InputLayout();
	
		il->inputBindingDesc.binding = 0;
		il->inputBindingDesc.stride = desc.vertexSize;
		il->inputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		for (uint32_t i = 0; i < desc.numElements; i++)
		{
			il->inputAttribDescs[i].binding = 0;
			il->inputAttribDescs[i].location = i;
			
			assert(desc.elements[i].numSubElements > 0);
			assert(desc.elements[i].type >= 0);
			assert(desc.elements[i].type < 7);

			il->inputAttribDescs[i].format = g_InputAttribFormatTable[desc.elements[i].type][desc.elements[i].numSubElements - 1];
			il->inputAttribDescs[i].offset = desc.elements[i].offset;
		}

		il->inputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		il->inputStateInfo.vertexBindingDescriptionCount = 1;
		il->inputStateInfo.pVertexBindingDescriptions = &il->inputBindingDesc;
		il->inputStateInfo.vertexAttributeDescriptionCount = desc.numElements;
		il->inputStateInfo.pVertexAttributeDescriptions = &il->inputAttribDescs[0];

		return il;
	}

	Shader* Device::CreateShader(const ShaderCreateDesc& desc)
	{
		Shader* shader = new Shader();

		//VkShaderModuleCreateInfo moduleCreateInfo{};
		//moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		//moduleCreateInfo.codeSize = desc.size;
		//moduleCreateInfo.pCode = (uint32_t*)desc.data;

		//memcpy(&shader->entryPoint[0], &desc.entryPoint[0], 16);

		//VK_CHECK_RESULT(vkCreateShaderModule(m_Device, &moduleCreateInfo, NULL, &shader->module));

		return shader;
	}

	Framebuffer* Device::DefaultFramebuffer()
	{
		return nullptr;
	}
}