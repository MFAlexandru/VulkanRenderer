#pragma once

#include <vulkan/vulkan.hpp>

class Texture2D
{
public:
	vk::Device* device;
	vk::Image               image;
	vk::ImageLayout         imageLayout;
	vk::DeviceMemory        deviceMemory;
	vk::ImageView           view;
	uint32_t              width, height;
	uint32_t              mipLevels;
	uint32_t              layerCount;
	vk::DescriptorImageInfo descriptor;
	vk::Sampler             sampler;

	void      updateDescriptor();
	void      destroy();

	void loadFromFile(
		std::string        filename,
		vk::Format           format,
		vk::Device* device,
		vk::Queue            copyQueue,
		vk::ImageUsageFlags  imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
		vk::ImageLayout      imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		bool               forceLinear = false);
};