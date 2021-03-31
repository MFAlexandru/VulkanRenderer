#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <math.h>
#include <fstream>
#include <array>
#include <unordered_map>
#include <string>

#include <chrono>

#include "VulkanRendererNeededBuildTypes.h"

class VulkanRenderer {
public:
	// ----- VARIABLES -----

	uint32_t WIDTH = 800;

	uint32_t HEIGHT = 600;

	#ifdef NDEBUG
		bool enableValidationLayers = false;
	#else
		bool enableValidationLayers = true;
	#endif
	// THOSE ARE NOT EXTENSIONS
	std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	std::string applicationName = "Noname";

	std::vector<const char*> instanceExtensions = {};

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	std::string TEXTURE_PATH = "../Textures/";

	std::string MODEL_PATH = "../Models/karanbit.obj";

	int MAX_FRAMES_IN_FLIGHT = 2;

	// CAMERA

	glm::vec3 cameraPos = glm::vec3(0.5f, 0.50f, 0.50f);
	glm::vec3 cameraFront = glm::normalize(-cameraPos);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	float yaw = -90.0f;
	float pitch = 0.0f;
	float fov = 45.0f;

	// MOUSE STATE
	float lastX = WIDTH / 2;
	float lastY = HEIGHT / 2;
	float sensitivity = 0.1;

	// TIME
	float deltaTime = 0;
	float lastFrame = 0;

	// ----- FUNCTIONS -----


	void initWindow();

	void createInstance();

	void setupDebugMessenger();

	void createSurface();

	void pickPhisicalDevice();

	void createLogicalDevice();

	void createSwapChain();

	void createImageViews();

	void createRenderPass();

	void createDescriptorSetLayout();

	void createGraphicsPipeline();

	void createCommandPool();

	void createDepthResources();

	void createFramebuffers();

	void createTextureImage();

	void createTextureImageView();

	void createTextureSampler();

	void loadModel();

	void createVertexBuffer();

	void createIndexBuffer();

	void createUniformBuffers();

	void createDescriptorPool();

	void createDescriptorSets();

	void createCommandBuffers();

	void createSyncObjects();

	void clean();

	void drawFrame();

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			// KEY BULLSHIT
			float cameraSpeed = 2.5 * deltaTime;
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				cameraPos += cameraSpeed * cameraFront;
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				cameraPos -= cameraSpeed * cameraFront;
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				cameraPos -= cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				cameraPos += cameraSpeed * glm::normalize(glm::cross(cameraFront, cameraUp));

			// MOUSE BULLSHIT
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			yaw += (xpos - lastX) * sensitivity;
			pitch += (lastY - ypos) * sensitivity;
			lastX = xpos;
			lastY = ypos;
			// CORRECT PITCH
			if (pitch > 89.0f) pitch = 89.0f;
			if (pitch < -89.0f) pitch = -89.0f;
			/*
			cameraFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront.y = sin(glm::radians(pitch));
			cameraFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(cameraFront);
			*/
			drawFrame();
		}

		device->waitIdle();
	}


	// ----- VARIABLES -----

	GLFWwindow* window;
	vk::UniqueInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;
	vk::UniqueDevice device;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Queue computeQueue;
	vk::SwapchainKHR swapChain;
	vk::SwapchainKHR swapChain2;
	std::vector<vk::Image> swapChainImages;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::ImageView> swapChainImageViews;
	vk::RenderPass renderPass;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline graphicsPipeline;
	vk::CommandPool commandPool;
	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	vk::Image textureImage;
	vk::DeviceMemory textureImageMemory;
	vk::ImageView textureImageView;
	vk::Sampler textureSampler;
	// MODEL
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	//------
	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferMemory;
	vk::Buffer indexBuffer;
	vk::DeviceMemory indexBufferMemory;
	std::vector<vk::Buffer> uniformBuffers;
	std::vector<vk::DeviceMemory> uniformBuffersMemory;
	vk::DescriptorPool descriptorPool;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;

	int currentFrame;
	bool framebufferResized = false;

	// ----- AUXILIAR FUNCTIONS -----
	std::vector<const char*> getRequiredExtensions();

	bool checkValidationLayerSupport();

	VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator,
		VkDebugUtilsMessengerEXT* pDebugMessenger);

	void DestroyDebugUtilsMessengerEXT(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks* pAllocator);

	bool isDeviceSuitable(const vk::PhysicalDevice &device);

	QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice &device);

	bool checkDeviceExtensionSupport(const vk::PhysicalDevice &device);

	SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice &device);

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

	vk::Format findDepthFormat();

	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void createImage(
		uint32_t width,
		uint32_t height,
		vk::Format format,
		vk::ImageTiling tiling,
		vk::ImageUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::Image& image,
		vk::DeviceMemory& imageMemory);

	void createBuffer(
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties,
		vk::Buffer& buffer,
		vk::DeviceMemory& bufferMemory);

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);

	vk::CommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

	void recreateSwapChain();

	void cleanupSwapChain();

	void updateUniformBuffer(uint32_t currentImage);

};