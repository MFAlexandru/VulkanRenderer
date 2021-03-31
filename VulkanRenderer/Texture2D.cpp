#include "Texture2d.h"

#include <stb_image.h>

void Texture2D::updateDescriptor() {

}

void Texture2D::destroy() {

    renderer->device->destroySampler(sampler);

    renderer->device->destroyImageView(view);

    renderer->device->destroyImage(image);

    renderer->device->freeMemory(deviceMemory);
}


void Texture2D::loadFromFile(
	std::string        filename,
	vk::Format           format,
	vk::ImageUsageFlags  imageUsageFlags,
	vk::ImageLayout      imageLayout,
	bool               forceLinear) {

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    width = texWidth;
    height = texHeight;
    layerCount = texChannels;

    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // PREPARE STAGING BUFFER
    // A BUFFER BASICALY INBETWEN
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    renderer->createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    // MAP MEMORY AND COPY DATA
    auto data = renderer->device->mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    renderer->device->unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    // OLD USAGE FLAG BITS vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
    // OLD FORMAT vk::Format::eR8G8B8A8Srgb

    renderer->createImage(texWidth, texHeight, format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | imageUsageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal, image, deviceMemory);
    // TRANZITIONING IMAGE LAYOUT BEFORE COPYING THE BUFFER
    renderer->transitionImageLayout(image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    renderer->copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    // TRANSITION NOW INTO A USEFULL LAYOUT
    renderer->transitionImageLayout(image, format, vk::ImageLayout::eTransferDstOptimal, imageLayout);


    // CLEANUP
    renderer->device->destroyBuffer(stagingBuffer, nullptr);
    renderer->device->freeMemory(stagingBufferMemory, nullptr);

    // CREATE IMAGE VIEW

    view = renderer->createImageView(image, format, vk::ImageAspectFlagBits::eColor);

    // CREATE SAMPLER

    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;

    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;

    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;

    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;

    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    mipLevels = 0;

    try {
        sampler = renderer->device->createSampler(samplerInfo);
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to create image sampler!"));
    }
}