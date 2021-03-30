#include "VulkanRenderer.h"

void VulkanRenderer::clean() {

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        device->destroySemaphore(renderFinishedSemaphores[i], nullptr);
        device->destroySemaphore(imageAvailableSemaphores[i], nullptr);
        device->destroyFence(inFlightFences[i], nullptr);
    }

    device->destroyDescriptorPool(descriptorPool);

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        device->destroyBuffer(uniformBuffers[i]);
        device->freeMemory(uniformBuffersMemory[i]);
    }

    device->destroyBuffer(indexBuffer);

    device->freeMemory(indexBufferMemory);

    device->destroyBuffer(vertexBuffer);

    device->freeMemory(vertexBufferMemory);

    device->destroySampler(textureSampler);

    device->destroyImageView(textureImageView);

    device->destroyImage(textureImage);

    device->freeMemory(textureImageMemory);

    device->destroyImageView(depthImageView);

    device->destroyImage(depthImage);

    device->freeMemory(depthImageMemory);

    device->destroyCommandPool(commandPool);

    for (auto framebuffer : swapChainFramebuffers) {
        device->destroyFramebuffer(framebuffer);
    }

    device->destroyPipeline(graphicsPipeline);

    device->destroyPipelineLayout(pipelineLayout);

    device->destroyDescriptorSetLayout(descriptorSetLayout);

    device->destroyRenderPass(renderPass);

    for (auto img : swapChainImageViews) {
        device->destroyImageView(img);
    }

    device->destroySwapchainKHR(swapChain, nullptr);

    instance->destroySurfaceKHR(surface, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(*instance, debugMessenger, nullptr);
    }

    glfwDestroyWindow(window);

    glfwTerminate();
}