#include "VulkanRenderer.h"

#include <iostream>


int main() {
    VulkanRenderer app;
    int wait;
    try {
        app.initWindow();
        app.createInstance();
        app.setupDebugMessenger();
        app.createSurface();
        app.pickPhisicalDevice();
        app.createLogicalDevice();
        app.createSwapChain();
        app.createImageViews();
        app.createRenderPass();
        app.createDescriptorSetLayout();
        app.createGraphicsPipeline();
        app.createCommandPool();
        app.createDepthResources();
        app.createFramebuffers();
        app.createTextureImage();
        app.createTextureImageView();
        app.createTextureSampler();
        app.loadModel();
        app.createVertexBuffer();
        app.createIndexBuffer();
        app.createUniformBuffers();
        app.createDescriptorPool();
        app.createDescriptorSets();
        app.createCommandBuffers();
        app.createSyncObjects();

        app.mainLoop();
        app.clean();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    std::cin >> wait;

    return EXIT_SUCCESS;
}