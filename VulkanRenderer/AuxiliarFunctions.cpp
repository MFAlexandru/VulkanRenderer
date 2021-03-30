#include "VulkanRenderer.h"

std::vector<const char*> VulkanRenderer::getRequiredExtensions() {
	// GET GLFW EXTENSION
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	for (const char* extension : instanceExtensions) {
		extensions.push_back(extension);
	}

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool VulkanRenderer::isDeviceSuitable(const vk::PhysicalDevice &device) {
	// FIND QUEUE FAMILIES
	QueueFamilyIndices indices = findQueueFamilies(device);

	// CHECK SUPPORT FOR EXTENSIONS
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	// CHECK THE SWAPCHAIN PROPRIETIES OF THE GPU
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	// CHECK ADITIONAL FEATURES OF THE GPU (NOT VULKAN EXTENSIONS)
	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(const vk::PhysicalDevice &device) {
	// CLASS TO REMEMBER WITCH QUEUE DOES WHAT
	QueueFamilyIndices indices;

	// GET THE TAGS OF THE QUEUES
	uint32_t queueFamilyCount = 0;
	auto queueFamilies = device.getQueueFamilyProperties();

	// VERIFY QUEUES FOR OUR NEEDS
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// CHECK GRAPHICS CAPABILITY
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		// CHECK COMPUTE CAPABILITY
		if (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
			indices.computeFamily = i;
		}

		// CHECK PRESENTATION CAPABILITy
		if (queueFamily.queueCount > 0 && device.getSurfaceSupportKHR(i, surface)) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}
	return indices;
}

bool VulkanRenderer::checkDeviceExtensionSupport(const vk::PhysicalDevice &device) {
	// QUERY FOR THE SUPPORTED EXTENSIOns
	auto availableExtensions = device.enumerateDeviceExtensionProperties();

	// VERIFY THAT YOU HAVE YOUR SPECIFIED EXTENSIONS
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
	// GET MEMORY PROPRIETIES
	auto  memProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanRenderer::createImage(
	uint32_t width, 
	uint32_t height, 
	vk::Format format, 
	vk::ImageTiling tiling,
	vk::ImageUsageFlags usage, 
	vk::MemoryPropertyFlags properties, 
	vk::Image& image, 
	vk::DeviceMemory& imageMemory) {

	vk::ImageCreateInfo imageInfo;
	imageInfo.imageType = vk::ImageType::e2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	// INITIAL LAYOUT UNDEFINED SO YOU CAN TRANSITION 
	// INTO ANYTHING
	imageInfo.initialLayout = vk::ImageLayout::eUndefined;
	imageInfo.usage = usage;
	imageInfo.samples = vk::SampleCountFlagBits::e1;
	imageInfo.sharingMode = vk::SharingMode::eExclusive;

	try {
		image = device->createImage(imageInfo);
	}
	catch (vk::SystemError err) {
		throw(std::runtime_error("failed to create Image!"));
	}
	// YOUR IMAGE APARENTY DOES NOT HAVE MEM EVEN TOUGH
	// IT IS CREATED SO YOU ALOCATE SOME
	// FIRST GET THE REQUIREMENTS OF THE IMAGE
	auto memRequirements = device->getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	// THERE ARE MORE TYPES OF MEMORY
	// SEE WITCH ONE YOU NEED
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	try {
		imageMemory = device->allocateMemory(allocInfo);
	}
	catch (vk::SystemError err) {
		throw(std::runtime_error("failed to create Image!"));
	}

	// THIRD PARAMETER IS OFFSET FROM MEM ADRESS
	device->bindImageMemory(image, imageMemory, 0);
}

vk::ImageView VulkanRenderer::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
	vk::ImageViewCreateInfo viewInfo;
	viewInfo.image = image;
	viewInfo.viewType = vk::ImageViewType::e2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	vk::ImageView imageView;

	try {
		imageView = device->createImageView(viewInfo);
	}
	catch (vk::SystemError err) {
		throw std::runtime_error("failed to create instance!");
	}

	return imageView;
}

void VulkanRenderer::createBuffer(
	vk::DeviceSize size, 
	vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags properties, 
	vk::Buffer& buffer, 
	vk::DeviceMemory& bufferMemory) {
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	try {
		buffer = device->createBuffer(bufferInfo);
	}
	catch (vk::SystemError err) {
		throw(std::runtime_error("failed to create buffer!"));
	}

	auto memRequirements = device->getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
	try {
		bufferMemory = device->allocateMemory(allocInfo);
	}
	catch (vk::SystemError err) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}
	device->bindBufferMemory(buffer, bufferMemory, 0);
}

vk::Format VulkanRenderer::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
	// CHECK FOR OPTIMAL FORMAT
	for (vk::Format format : candidates) {
		auto props = physicalDevice.getFormatProperties(format);
		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanRenderer::findDepthFormat() {
	// FIND THE BEST FORMAT FOR OUR CASE
	return findSupportedFormat(
		{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint,},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
}

std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

vk::ShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	// CREATE SHADER MODULES

	vk::ShaderModule shaderModule;
	try {
		shaderModule = device->createShaderModule(createInfo);
	}
	catch (vk::SystemError err) {
		throw std::runtime_error("failed to create instance!");
	}

	return shaderModule;
}

void VulkanRenderer::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	// STUFF TO DEAL WITH UNIDENTIFIED LAYOUT AND OTHER LAYOUTS
	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;
	// SRC -> FROM WHERE I TAKE DATA, WHAT THE "STAGE" (NOT PIPELINE STAGE)
	// FORM BEFORE (OR JUST WAS) DID SO I SHOULD WAIT UNTIL IS FINISHED TO USE IT
	// DST -> WHAT THIS STAGE WILL DO, ACCESS ETC
	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = {};
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	commandBuffer.pipelineBarrier(sourceStage, destinationStage,
		{},
		0, nullptr,
		0, nullptr,
		1, &barrier);


	endSingleTimeCommands(commandBuffer);
}

// CREATION OF SMALL COMMAND BUFFERS

vk::CommandBuffer VulkanRenderer::beginSingleTimeCommands() {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;


	vk::CommandBuffer commandBuffer = device->allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	graphicsQueue.submit(1, &submitInfo, vk::Fence());

	graphicsQueue.waitIdle();

	device->freeCommandBuffers(commandPool, 1, &commandBuffer);
}

void VulkanRenderer::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	commandBuffer.copyBufferToImage(
		buffer,
		image,
		vk::ImageLayout::eTransferDstOptimal,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferCopy copyRegion{};
	copyRegion.size = size;

	commandBuffer.copyBuffer(
		srcBuffer,
		dstBuffer,
		1,
		&copyRegion
	);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::recreateSwapChain() {
	// For window resize
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	device->waitIdle();

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();
}

void VulkanRenderer::cleanupSwapChain() {
	// DESTROY DEPTH BUFFER
	device->destroyImageView(depthImageView, nullptr);
	device->destroyImage(depthImage, nullptr);
	device->freeMemory(depthImageMemory, nullptr);

	// Clean the swapchain for resize
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		device->destroyFramebuffer(swapChainFramebuffers[i], nullptr);
	}

	// Clean descriptor pool
	device->destroyDescriptorPool(descriptorPool, nullptr);

	device->freeCommandBuffers(commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

	device->destroyPipeline(graphicsPipeline, nullptr);
	device->destroyPipelineLayout(pipelineLayout, nullptr);
	device->destroyRenderPass(renderPass, nullptr);

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		device->destroyImageView(swapChainImageViews[i], nullptr);
	}

	device->destroySwapchainKHR(swapChain, nullptr);

	// CLEAN THE UNIFORM BUFFER
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		device->destroyBuffer(uniformBuffers[i], nullptr);
		device->freeMemory(uniformBuffersMemory[i], nullptr);
	}
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
	// CHECK THE TIME
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	// MODEL TRANSFORM
	UniformBufferObject ubo{};
	ubo.model = glm::identity<glm::mat4>();
	ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(pitch), /* glm::vec3(0.0f, 0.0f, 1.0f) */ glm::normalize(glm::cross(cameraFront, cameraUp)));
	ubo.model = glm::rotate(ubo.model, glm::radians(yaw), /* glm::vec3(0.0f, 1.0f, 0.0f) */ cameraUp);
	ubo.view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	ubo.proj = glm::perspective(glm::radians(fov), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	// Copy data
	auto data = device->mapMemory(uniformBuffersMemory[currentImage], 0, sizeof(ubo));
	memcpy(data, &ubo, sizeof(ubo));
	device->unmapMemory(uniformBuffersMemory[currentImage]);
}