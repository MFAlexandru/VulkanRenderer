#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "VulkanRenderer.h"
#include <filesystem>

void VulkanRenderer::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Not using OpenGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // No resize

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanRenderer::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo;
    // VALIDATION ERROR
    appInfo.pNext = NULL;
    appInfo.pApplicationName = applicationName.data();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.pEngineName = NULL;
    appInfo.engineVersion = VK_MAKE_VERSION(1, 2, 0);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 0);

    // INSTANCE INFO STRUCT
    vk::InstanceCreateInfo createInfo;
    createInfo.pNext = NULL;
    createInfo.pApplicationInfo = &appInfo;

    // TELL VULKAN WITCH EXTENSIONS WE NEED
    auto extensionsforLayers = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsforLayers.size());
    createInfo.ppEnabledExtensionNames = extensionsforLayers.data();

    // PRINT AVAILABLE EXTENSIONS TO THE SCREEN
    // GENERALLY WHEN YOU NEED TO QUERY THE DEVICE FOR
    // PROPRIETIS OR STUFF YOU WILL USE A SEQUENCE LIKE THE
    // ONE DESCRIBED BELOW

    auto extensions = vk::enumerateInstanceExtensionProperties();

    std::cout << "available extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }

    // Validation layer stuff -----------------------------
    {
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

        }
        else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }
    }
    // Done -----------------------------------------------

    // CREATE INSTANCE
    try {
        instance = vk::createInstanceUnique(createInfo, nullptr);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create instance!");
    }
}

// USED IN THE NEXT FUNCTION ---------- UTILITARY
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void VulkanRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    auto createInfo = vk::DebugUtilsMessengerCreateInfoEXT(
        vk::DebugUtilsMessengerCreateFlagsEXT(),
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debugCallback,
        nullptr
    );

    if (CreateDebugUtilsMessengerEXT(*instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*> (&createInfo), nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanRenderer::createSurface() {
    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = rawSurface;
}

void VulkanRenderer::pickPhisicalDevice() {

    // GET DEVICE NAMES
    auto devices = instance->enumeratePhysicalDevices();

    if (devices.size() == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // SEE WHAT DEVICE TO USE
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // Most extensions and device proprieties are controled from here
    // extensions are named proprieties

    // STRUCTURE FOR QUEUES
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.computeFamily.value() };

    // FOR NOW ALL PRIORITES ARE EQUAL
    // FOR FUTURE MAKE DIFFERENT PRIORITIES
    // DEPENDING ON WHAT YOU WHANT FROM YOUR QUEUE
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // STRUCT FOR DEVICE FEATURES
    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = VK_TRUE; 

    // STRUCT FOR DEVICE INFO
    vk::DeviceCreateInfo createInfo;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        createInfo.enabledLayerCount = 0;
    }
    try {
        device = physicalDevice.createDeviceUnique(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create Logical device!");
    }

    graphicsQueue = device->getQueue( indices.graphicsFamily.value(), 0);
    presentQueue = device->getQueue(indices.presentFamily.value(), 0);
    computeQueue = device->getQueue(indices.computeFamily.value(), 0);
}

void VulkanRenderer::createSwapChain() {
    // GET THE DETAILS OF WHAT CAN YOU DO WITH THE SWAPCHAIN
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    // CHOSE THE REQUIRED PROPRIETIES
    auto surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    auto presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    auto extent = chooseSwapExtent(swapChainSupport.capabilities);

    // SET THE IMAGE COUNT FOR BAKBF
    auto imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 
        && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // STRUCT FOR THE SWAP CHAIN
    vk::SwapchainCreateInfoKHR createInfo;
    createInfo.surface = surface;

    // SPECIFY FORMAT EXTENT
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    auto indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // SPECIFY THE SHARING MODE FOR THE QUEUES IN CASE GRAPHICS
    // AND PRESENT ARE NOT THE SAME
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // SPECIFY A TRANSFORMATION, HERE WE DO NOT USE ONE
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

    // COLOR OF ALPHA CHANEL (IGNORE IT)
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    // SPECIFY PR MODE
    createInfo.presentMode = presentMode;
    createInfo.clipped = true;

    createInfo.oldSwapchain = nullptr;

    try {
        swapChain = device->createSwapchainKHR(createInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create swap Chain!");
    }

    // RETRIVE IMAGE HANDELS FORMATS AND EXTENT
    swapChainImages = device->getSwapchainImagesKHR(swapChain);

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanRenderer::createImageViews() {
    // CREATE VIEWS FOR THE RETRIVED IMAGES
    // THE IMAGES ARE CREATED ON DEVICE MEM WHEN YOU CREATE
    // THE SWAPCHAIN
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor);
    }
}

void VulkanRenderer::createRenderPass() {

    // DESCRIBE DEPTH ATTACHMENT
    // THE LITERAL DEPTH TESING IS DONE BY ITSELF, I THINK
    // YOU CAN CHANGE THAT
    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // DESCRIBE COLOR ATTACHMENT
    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // DESCRIBE A REFERENCE TO THE COLORATTACHMENT 
    // JUST SPECIFY THE INDEX OF THE ATTACHMENT
    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    // DESCRIBE SUBPASSES
    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // DEPENDENCY BEFORE AND AFTER RENDER PASS STARTS
    vk::SubpassDependency dependency;
    // SUBPASS BEFORE THIS SUBPASS
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // THIS SUBPASS (INDEX IN SUBPASS ARRAY)
    dependency.dstSubpass = 0;
    // WHAT THE SUBPASS BEFORE DID OR MODIFIED,
    // WHAT TO WAIT ON UNTIL I START BASICALLY
    // WHAT STAGES OF THE PIPELINE WILL THE SUBPASS BEFORE PRODUCE DATA FOR
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    // WHAT THIS SUBPASS WILL DO, MODIFY ..
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    // DESCRIBE ACTUAL RENDER PASS
    // USE THESE INDEXES OF THE ATTACHMENTS
    // INT THE ATTACHMENTREFERENCE STRUCT
    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    try {
        renderPass = device->createRenderPass(renderPassInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create Render Pass!");
    }
}

void VulkanRenderer::createDescriptorSetLayout() {
    // DESCRIBE THE BINDINGS TO THE SHADER
    // HERE YOU SAY WHERE YOU BIND THE LAYOUT
    // THESE ARE NOT THE "MANATORY" INPUTS
    // I THINK BINDINGS MUST NOT OVERLAP INBETWEN STAGES
    vk::DescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    // MAKE ANOTHER LAYOUT BINDING FOR THE IMAGES
    vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;

    // HERE IS THE ACTUAL LAYOUT 
    // (BINDINGS CONTAIN SETS)
    // (LAYOUTS CONTAIN BINDINGS)
    vk::DescriptorSetLayoutCreateInfo layoutInfo;
    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    try {
        descriptorSetLayout = device->createDescriptorSetLayout(layoutInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create Descriptor Set Layout!");
    }
}

// UUSED IN NEXT FUNCTION
std::vector<char> readFile(const std::string& filename);

void VulkanRenderer::createGraphicsPipeline() {
    // READ BINARY SHADER FILES
    //std::system("./compile.bat");
    auto vertShaderCode = readFile("C:/Users/Alex/Desktop/VulkanRenderer/VulkanRenderer/Shaders/vert.spv");
    auto fragShaderCode = readFile("C:/Users/Alex/Desktop/VulkanRenderer/VulkanRenderer/Shaders/frag.spv");

    vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // FILL CREATE INFO FOR VERTEX
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    // FILL STRUCT FOR FRAGMENT
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    // LIST OF MY SHADER STAGES
    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // DESCRIBE VERTEX DATA FORMAT
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    // GET THE BINDING AND HOW THE STRUCT LOOKS

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // WHAT KIND OF GEOMETRY WE HAVE
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // DESCRIBE VIEWPORT
    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // DESCRIBE SCISSOR RECTANGLE
    vk::Rect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    // COMBINE THE TWO ABOVE IN A VIEWPORT STATE
    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // DESCRIBE RASTERIZER
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // DESCRIBE MULTISAMPLER
    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // DESCRIBE ATTACHED COLORBLENDER
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne; // Optional
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero; // Optional
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd; // Optional

                                                         // DESCRIBE GLOBAL COLORBLEND
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

                                            // FILL DINAMIC STATE INFO
    vk::DynamicState dynamicStates[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eLineWidth
    };

    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // DESCRIBE AND MAKE PIPELINE LAYOUT
    // HOW THE MEMORY LAYOUT LOOKS
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    try {
        pipelineLayout = device->createPipelineLayout(pipelineLayoutInfo);
    }
    catch (vk::SystemError err) {
        throw std::runtime_error("failed to create Pipeline Layout!");
    }

    // USE DEPTH BUFFER SPEC
    // HERE YOU ENABLE SOME STUFF FOR DEPTH BUFF
    // I THINK
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;

    depthStencil.depthCompareOp = vk::CompareOp::eLess;

    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional

    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    // DESCRIBE THE PIPELINE
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    // ADD THE OPTIONS WE HAVE SET UNTIL NOW
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional

    pipelineInfo.layout = pipelineLayout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.basePipelineHandle = nullptr; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    vk::Result   result;
    std::tie(result, graphicsPipeline) = device->createGraphicsPipeline(nullptr, pipelineInfo);

    switch (result)
    {
    case vk::Result::eSuccess: break;
    case vk::Result::ePipelineCompileRequiredEXT:
        throw std::runtime_error("failed to create Pipeline!");
        break;
    default: assert(false);  // should never happen
    }

    device->destroyShaderModule(fragShaderModule);
    device->destroyShaderModule(vertShaderModule);
}

void VulkanRenderer::createCommandPool() {
    // JUST CHUCK THE INDICES IN THE CREATE INFO AND U ARE GOOD IT SEEMS
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    // STRUCT FOR COMMAND POOLS
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = {}; // Optional

    try {
        commandPool = device->createCommandPool(poolInfo);
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to create Command Pool!"));
    }
}

void VulkanRenderer::createDepthResources() {
    auto depthFormat = findDepthFormat();

    // CREATE THE IMAGE
    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
    // CREATE IMAGE VIEW
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

void VulkanRenderer::createFramebuffers() {
    // You need as many buffers as images on swapchain
    swapChainFramebuffers.resize(swapChainImageViews.size());

    // CREATE A FRAME BUFFER FOR EACH IMAGE IN THE SWAPCHAIN
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<vk::ImageView, 2> attachments = {
        swapChainImageViews[i],
        depthImageView
        };

        // FRAME BUFFERURILE SE CREEAZA DIFF DE RESTUL CRED
        vk::FramebufferCreateInfo framebufferInfo;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        try {
            swapChainFramebuffers[i] = device->createFramebuffer(framebufferInfo);
        }
        catch (vk::SystemError err) {
            throw(std::runtime_error("failed to create frame buffer!"));
        }
    }
}

void VulkanRenderer::createTextureImage() {
    // LOAD IMG
    int texWidth, texHeight, texChannels;
    std::vector<std::string> texturesPaths;

    for (const auto& entry : std::filesystem::directory_iterator(TEXTURE_PATH))
        texturesPaths.push_back(entry.path().string());
    std::cout << texturesPaths[0];
    stbi_uc* pixels = stbi_load(texturesPaths[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // PREPARE STAGING BUFFER
    // A BUFFER BASICALY INBETWEN
    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;

    createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc , vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    // MAP MEMORY AND COPY DATA
    auto data = device->mapMemory(stagingBufferMemory, 0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    device->unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);
    // TRANZITIONING IMAGE LAYOUT BEFORE COPYING THE BUFFER
    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    // TRANSITION NOW INTO A USEFULL LAYOUT
    transitionImageLayout(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);


    // CLEANUP
    device->destroyBuffer(stagingBuffer, nullptr);
    device->freeMemory(stagingBufferMemory, nullptr);
}

void VulkanRenderer::createTextureImageView() {
    textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

void VulkanRenderer::createTextureSampler() {
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

    try {
        textureSampler = device->createSampler(samplerInfo);
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to create image sampler!"));
    }
}

void VulkanRenderer::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            if (index.texcoord_index > 0)
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1 - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);

        }
    }
    std::cout << vertices.size();
}

void VulkanRenderer::createVertexBuffer() {
    // ANALOG WITH CREATE TEXTURE IMAGE, FOR EXPLINATIONS 
    // GO THERE
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    auto data = device->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    device->unmapMemory(stagingBufferMemory);


    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    device->destroyBuffer(stagingBuffer, nullptr);
    device->freeMemory(stagingBufferMemory, nullptr);
}

void VulkanRenderer::createIndexBuffer() {
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    auto data = device->mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    device->unmapMemory(stagingBufferMemory);

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    device->destroyBuffer(stagingBuffer, nullptr);
    device->freeMemory(stagingBufferMemory, nullptr);
}

void VulkanRenderer::createUniformBuffers() {
    vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

    // MAKES SENSE BECAUSE THESE WILL BE DIFFERENT DEPENDING
    // ON WITCH POS IS THE CAMERA AND SHIT
    uniformBuffers.resize(swapChainImages.size());
    uniformBuffersMemory.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void VulkanRenderer::createDescriptorPool() {

    // YOU ALOCATE DESCRIPTORS ONLU FOR THE IMAGE AND THE UNIFORS
    // FOR INDEXES AND VERTEXES YOU DO NOT NEED ONE
    // THE TEXTURE DOESNT CHANGE SO FOR IT WE HAVE ONLY ONE ALLOCATION
    // INSTEAD OF THE NUMBER FO SWAP CHAINS LIKE FOR THE UNIFORMS
    // THIS DOWN DESCRIBES WHAT TIPE OF DESCRIPTORS AND HOW MANY TO MAKE OF EACH
    std::array<vk::DescriptorPoolSize, 2> poolSizes;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // YOU BASICALLY NOW CAN MAKE MORE OF EACH LAYOUT THAT YOU MADE EARLIER, I THINK

    // DESCRIPTION OF DESCRIPTOR POOL
    vk::DescriptorPoolCreateInfo poolInfo;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    // MAXIMUM NUMBER FROM THE ONES DECIDET HIGHER
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    try {
        descriptorPool = device->createDescriptorPool(poolInfo);
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to create descriptor pool!"));
    }
}


void VulkanRenderer::createDescriptorSets() {
    // ONE DESCRIPTOR SET LAYOUT FOR EACH SWAP CHAIN IMAGE
    // DUPLICATE THE ONE MADE EARLYER
    std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    try {
        descriptorSets = device->allocateDescriptorSets(allocInfo);
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to allocate descriptor sets!"));
    }

    // PUT DATA IN THE DESCRIPTOR SETS
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // UNIFORM DATA SOURCE
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        // IMAGE DATA SOURCE
        vk::DescriptorImageInfo imageInfo;
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<vk::WriteDescriptorSet, 2> descriptorWrites;
        // A DESCRIPTOR SET CONSISTS OF ONE OF EACH OF THESE TWO

        // UNIFORM DATA DESTINATION
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // IMAGE DATA DESTINATION
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        device->updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

}

void VulkanRenderer::createCommandBuffers() {

    // STRUCT FOR COMMAND BUFFERS
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.commandPool = commandPool;
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandBufferCount = (uint32_t)swapChainImages.size();

    try {
        commandBuffers = device->allocateCommandBuffers(allocInfo);
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to allocate command buffers!"));
    }

    // START RECORDING THE COMMAND BUFFER FOR EACH FRAME
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        // YOU CAN ADD FALGS FOR USE AND SOMETHING WITH INHERETANCE
        vk::CommandBufferBeginInfo beginInfo;

        commandBuffers[i].begin(beginInfo);

        // START RENDER PASS
        vk::RenderPassBeginInfo renderPassInfo;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        // ONE CLEAR VALUE FOR THE IMAGE AND ONE FOR THE
        // DEPTH STENCIL
        std::array<vk::ClearValue, 2> clearValues;
        clearValues[0].color.setFloat32({ 0.0f, 0.0f, 0.0f, 1.0f });
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        commandBuffers[i].beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

        commandBuffers[i].bindPipeline( vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        vk::Buffer vertexBuffers[] = { vertexBuffer };
        vk::DeviceSize offsets[] = { 0 };

        commandBuffers[i].bindVertexBuffers( 0, 1, vertexBuffers, offsets);

        commandBuffers[i].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint32);

        commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        commandBuffers[i].endRenderPass();

        try {
            commandBuffers[i].end();
        }
        catch (vk::SystemError err) {
            throw(std::runtime_error("failed to end the command buffer recording!"));
        }
    }
}

void VulkanRenderer::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    // WILL NEED THIS ONE LATER
    imagesInFlight.resize(swapChainImages.size(), nullptr);

    vk::SemaphoreCreateInfo semaphoreInfo;

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    try {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            imageAvailableSemaphores[i] = device->createSemaphore(semaphoreInfo, nullptr);
            renderFinishedSemaphores[i] = device->createSemaphore(semaphoreInfo, nullptr);
            inFlightFences[i] = device->createFence(fenceInfo, nullptr);
        }
    }
    catch (vk::SystemError err) {
        throw(std::runtime_error("failed to create sync obj!"));
    }
}

void VulkanRenderer::drawFrame() {
    // IF OUR FRAME IS IN FLIGHT WAIT FOR IT
    device->waitForFences( 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    
    uint32_t imageIndex;
    // AQUIRE AN IMAGE FROM THE SWAP CHAIN AND SET THE SEMAPHORE THAT WILL BE 
    // SIGNALED WHEN PRESENTATION HAS FINISHED READING FROM IMAGE
    auto result = device->acquireNextImageKHR(swapChain, static_cast<uint64_t>(UINT64_MAX), imageAvailableSemaphores[currentFrame], nullptr, &imageIndex);

    // CASE OF RESIZED WINDOW
    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateSwapChain();
        return;
    }
    else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // CHECK IF PREVIOUS FRAME IS USING THE IMAGE WE WANT TO ACCESS
    // FROM WHAT I UNDERSTAND THE SEMAPHORE USED EARLYER DOES THE
    // SAME THING BUT IDUNO MAN
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        device->waitForFences(1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // MARK THE IMAGE AS BEING USED BY THIS FRAME
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
    // UPDATE THE UNIFORM BUFFER
    updateUniformBuffer(imageIndex);

    // SUBMIT COMMAND BUFFER INFO
    vk::SubmitInfo submitInfo;
    // PUT THE SEMAPHORE THAT WE HAD SET TO BE SIGNALED
    // SO WE KNOW WHEN TO START
    vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    // SEMAPHORE TO BE SIGNALED WHEN ALL RENDERING HAS FINISHED AND PRESENTATION CAN BEGIN
    vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // RESET THE IN FLIGHT FENCE BECAUSE NOW WE WILL USE THIS FENCE
    device->resetFences(1, &inFlightFences[currentFrame]);

    // SUBMIT AND CHOSE THE FENCE THAT WILL BE SET WHEN FRAME HAS FINISHED
    if (graphicsQueue.submit(1, &submitInfo, inFlightFences[currentFrame]) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // PRESENTATION INFO
    vk::PresentInfoKHR presentInfo;

    presentInfo.waitSemaphoreCount = 1;
    // THIS IS THE RENDER FINISH SEMAPHOER USED EARLYER
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    presentInfo.pResults = nullptr; // Optional

    result = presentQueue.presentKHR(&presentInfo);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // INCREASE THE CURRENT FRAME
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}