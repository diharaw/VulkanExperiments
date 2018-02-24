#pragma once

#include <vulkan/vulkan.h>

#define DW_VK_MAX_INPUT_ATTRIB 8
#define DW_VK_MAX_RENDER_TARGETS 16

namespace gfx
{
	struct Fence
	{
		VkFence fence;
	};

	struct Texture
	{
		VkImage		   image;
		VkImageView    imageView;
		VkDeviceMemory memory;
	};

	struct Texture1D : Texture
	{

	};

	struct Texture2D : Texture
	{
		uint16_t width;
		uint16_t height;
	};

	struct Texture3D : Texture
	{

	};

	struct TextureCube : Texture
	{

	};

	struct BufferCreateDesc
	{

	};

	struct VertexBuffer
	{
		VkBuffer	   buffer;
		VkDeviceMemory memory;
	};

	struct IndexBuffer
	{
		VkBuffer	   buffer;
		VkDeviceMemory memory;
		uint32_t	   dataType;
	};

	struct ConstantBuffer
	{
		VkBuffer	   buffer;
		VkDeviceMemory memory;
	};

	struct InputElementDesc
	{
		uint32_t	numSubElements;
		uint32_t    type;
		bool		normalized;
		uint32_t	offset;
		const char* semanticName;
	};

	struct InputLayoutCreateDesc
	{
		InputElementDesc* elements;
		uint32_t		  vertexSize;
		uint32_t		  numElements;
	};

	struct InputLayout
	{
		VkVertexInputBindingDescription		 inputBindingDesc;
		VkVertexInputAttributeDescription	 inputAttribDescs[DW_VK_MAX_INPUT_ATTRIB];
		VkPipelineVertexInputStateCreateInfo inputStateInfo;
	};

	struct VertexArray
	{
		VertexBuffer* vertexBuffer;
		IndexBuffer*  indexBuffer;
		InputLayout*  layout;
	};

	struct ShaderCreateDesc
	{
		uint32_t    type;
		size_t		size;
		void*		data;
		char		entryPoint[16];
	};

	struct FramebufferCreateDesc
	{
		uint32_t numRenderTargets;
		Texture* renderTargets[DW_VK_MAX_RENDER_TARGETS];
		Texture* depthStencilTexture;
	};

	struct Texture2DCreateDesc
	{

	};

	struct PipelineStateCreateDesc
	{

	};

	struct DescriptorHeapCreateDesc
	{

	};

	struct DescriptorSetCreateDesc
	{

	};

	struct Shader
	{
		VkShaderModule m_VKModule;
		char		   m_EntryPoint[16];
	};

	struct PipelineState
	{
		VkPipeline m_Pipeline;
	};
	
	struct DescriptorSet
	{

	};

	struct DescriptorHeap
	{

	};

	struct Framebuffer
	{
		uint16_t	  m_NumRenderTargets;
		VkFramebuffer m_VKFramebuffer;
	};

	struct CommandQueue
	{
		VkQueue queue;
	};

	class CommandBuffer
	{
	private:
		VkCommandBuffer m_VKCommandBuffer;
	};

	class Device
	{
	private:
		VkDevice m_VKDevice;
		Framebuffer* m_SwapChainFramebuffers;

	public:
		bool Init();

		// Creation
		InputLayout* CreateInputLayout(const InputLayoutCreateDesc& desc);
		Shader* CreateShader(const ShaderCreateDesc& desc);
		PipelineState* CreatePipelineState(const PipelineStateCreateDesc& desc);
		DescriptorHeap* CreateDescriptorHeap(const DescriptorHeapCreateDesc& desc);
		DescriptorSet* CreateDescriptorSet(const DescriptorSetCreateDesc& desc);
		Texture2D* CreateTexture2D(const Texture2DCreateDesc& desc);
		Framebuffer* CreateFramebuffer(const FramebufferCreateDesc& desc);

		Framebuffer* DefaultFramebuffer();
	};
}
