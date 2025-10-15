#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_VULKAN
#define CIMGUI_USE_GLFW
#include "external/cimgui.h"
#include "external/cimgui_impl.h"

struct VkState{
    struct Resolution{
        uint32_t width;
        uint32_t height;
    }resolution;
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    float queuePriority;
    struct QueueFamilyIndices{
        uint32_t graphicsFamily;
        uint32_t computeFamily; }queueFamilyIndices;
    struct QueueCount{
        uint32_t graphics;
        uint32_t compute;
    }queueCount;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue computeQueue;
    VkSurfaceKHR surface;
    struct surfaceInfo{
        VkSurfaceCapabilitiesKHR capabilities;
        VkSurfaceFormatKHR surfaceFormat;
        VkPresentModeKHR presentMode;
    }surfaceInfo;
    VkExtent2D extent;
    uint32_t imageCount;
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkShaderModule vertexShader;
    VkShaderModule fragmentShader;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkFramebuffer* swapchainFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* inFlightFences;
    uint32_t MAX_FRAMES_IN_FLIGHT;
    uint32_t currentFrame;
    bool framebufferResized;

    VkDescriptorPool imguiDescriptorPool;

    VkDebugUtilsMessengerEXT debugMessenger;
};

void cleanup(struct VkState* state);

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
        VkDebugUtilsMessageTypeFlagsEXT messageType, 
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
        void* pUserData){
    printf("validation layer: %s \n", pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, 
        VkDebugUtilsMessengerEXT* pDebugMessenger){
    PFN_vkCreateDebugUtilsMessengerEXT func = 
        (PFN_vkCreateDebugUtilsMessengerEXT) 
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if(func != NULL){
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void setupDebugMessenger(struct VkState* state){
    printf("[+] Setting up Debug Messenger\n");
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,

        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,

        .messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = NULL
    };

    assert(CreateDebugUtilsMessengerEXT(state->instance, &createInfo, 
                NULL, &state->debugMessenger) == VK_SUCCESS);
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator){
   PFN_vkDestroyDebugUtilsMessengerEXT func = 
       (PFN_vkDestroyDebugUtilsMessengerEXT)
       vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != NULL) { func(instance, debugMessenger, pAllocator); }
}

bool checkValidationLayerSupport(){
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    bool layerFound = false;
    for(uint32_t i = 0; i < layerCount; i++){
        if(strcmp(availableLayers[i].layerName, validationLayer) == 0){
            printf("[+] Found validation layer %s \n", validationLayer);
            layerFound = true;
            break;
        }
    }

    return layerFound;
}

static void frameBufferResizeCallback(GLFWwindow* window, int width, int height){
    struct VkState* temp = (struct VkState*)glfwGetWindowUserPointer(window);
    temp->framebufferResized = true;
}

void initWindow(struct VkState* state){
    printf("[+] Initializing Window\n");
    glfwInit();
    state->resolution.width  = 800;
    state->resolution.height = 600;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    state->window = glfwCreateWindow(state->resolution.width, 
            state->resolution.height, "Vulkan", NULL, NULL);
    glfwSetWindowUserPointer(state->window, state);
    glfwSetFramebufferSizeCallback(state->window, frameBufferResizeCallback);
    assert(state->window);
}

void createInstance(struct VkState* state){
    printf("[+] Creating Vulkan Instance\n");
    assert(checkValidationLayerSupport);
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "null",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    VkExtensionProperties extensions[extensionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
    for(uint32_t i = 0; i < extensionCount; i++){
        printf("[!] Found: %s \n", extensions[i].extensionName);
    }
    uint32_t glfwExtensionsCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    uint32_t enabledExtensionCount = glfwExtensionsCount + 1;
    const char* enabledExtensions[enabledExtensionCount];
    memcpy(enabledExtensions, glfwExtensions, sizeof(char*) * glfwExtensionsCount);
    enabledExtensions[glfwExtensionsCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    uint32_t validationLayerCount = 1;
    const char * validationLayers[validationLayerCount];
    validationLayers[0] = "VK_LAYER_KHRONOS_validation";

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = enabledExtensionCount,
        .ppEnabledExtensionNames = enabledExtensions,
        .enabledLayerCount = validationLayerCount,
        .ppEnabledLayerNames = validationLayers
    };

    VkResult result = vkCreateInstance(&createInfo, NULL, &state->instance);
    assert(result == VK_SUCCESS);
}

void pickPhysicalDevice(struct VkState* state){
    printf("[+] Picking Physical Device\n");
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(state->instance, &deviceCount, NULL);
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(state->instance, &deviceCount, devices);
    state->physicalDevice = devices[0];

    vkGetPhysicalDeviceProperties(state->physicalDevice, &state->physicalDeviceProperties);
    printf("[+] Using Device %s\n", state->physicalDeviceProperties.deviceName);

    vkGetPhysicalDeviceFeatures(state->physicalDevice, &state->physicalDeviceFeatures);
}

void setupQueues(struct VkState* state){
    printf("[+] Setting up Queues\n");
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &queueFamilyCount, queueFamilyProperties);
    for(int i = 0; i < queueFamilyCount; i++){
        if((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)){
            if(queueFamilyProperties[i].queueCount > state->queueCount.graphics){
                state->queueFamilyIndices.graphicsFamily = i;
                state->queueCount.graphics = queueFamilyProperties[i].queueCount;
            }
        }
        if((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)){
            if(queueFamilyProperties[i].queueCount > state->queueCount.compute){
                state->queueFamilyIndices.computeFamily = i;
                state->queueCount.compute = queueFamilyProperties[i].queueCount;
            }
        }
    }
    printf("[+] Graphics using Queue Family [ %i ] with [ %i ] queues\n", state->queueFamilyIndices.graphicsFamily, state->queueCount.graphics);
    printf("[+] Compute using Queue Family  [ %i ] with [ %i ] queues\n", state->queueFamilyIndices.computeFamily, state->queueCount.compute);
}

void createLogicalDevice(struct VkState* state){
    printf("[+] Creating Logical Device\n");
    VkDeviceQueueCreateInfo queueCreateInfos[2];
    uint32_t queueCreateInfoCount = 0;
    state->queuePriority = 1.0f;

    VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = state->queueFamilyIndices.graphicsFamily,
        .queueCount = state->queueCount.graphics,
        .pQueuePriorities = &state->queuePriority
    };
    queueCreateInfos[0] = graphicsQueueCreateInfo;
    queueCreateInfoCount++;

    if(state->queueFamilyIndices.computeFamily != state->queueFamilyIndices.graphicsFamily){
        VkDeviceQueueCreateInfo computeQueueCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = state->queueFamilyIndices.computeFamily,
            .queueCount = state->queueCount.compute,
            .pQueuePriorities = &state->queuePriority
        };
        queueCreateInfos[1] = computeQueueCreateInfo;
        queueCreateInfoCount++;
    }

    const char * enabledExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, };
    uint32_t enabledExtensionCount = 1;

    VkPhysicalDeviceFeatures enabledFeatures = {};

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    uint32_t enabledLayerCount = 1;

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = queueCreateInfoCount,
        .ppEnabledExtensionNames = enabledExtensions,
        .enabledExtensionCount = enabledExtensionCount,
        .pEnabledFeatures = &enabledFeatures,
        .ppEnabledLayerNames = &validationLayer,
        .enabledLayerCount = enabledLayerCount,
    };
    assert(vkCreateDevice(state->physicalDevice, &createInfo, NULL, &state->device) == VK_SUCCESS);
}

void retrieveQueues(struct VkState* state){
    printf("[+] Retrieving Queues\n");
    vkGetDeviceQueue(state->device, state->queueFamilyIndices.graphicsFamily, 0, &state->graphicsQueue);
    vkGetDeviceQueue(state->device, state->queueFamilyIndices.computeFamily, 0, &state->computeQueue);
}

void createSurface(struct VkState* state){
    printf("[+] Creating Surface\n");
    assert(glfwCreateWindowSurface(state->instance, state->window, NULL, &state->surface) == VK_SUCCESS);
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(state->physicalDevice, state->queueFamilyIndices.graphicsFamily, state->surface, &presentSupport);
    assert(presentSupport);
};

static inline int clamp(int input, int min, int max){
    return input < min ? min : (input > max ? max : input);
}

static void destroyRenderFinishedSemaphores(struct VkState* state){
    if(state->renderFinishedSemaphores == NULL){
        return;
    }

    for(uint32_t i = 0; i < state->imageCount; i++){
        if(state->renderFinishedSemaphores[i] != VK_NULL_HANDLE){
            vkDestroySemaphore(state->device, state->renderFinishedSemaphores[i], NULL);
        }
    }

    free(state->renderFinishedSemaphores);
    state->renderFinishedSemaphores = NULL;
}

static void createRenderFinishedSemaphores(struct VkState* state){
    assert(state->renderFinishedSemaphores == NULL);
    if(state->imageCount == 0){
        return;
    }

    state->renderFinishedSemaphores = malloc(state->imageCount * sizeof(VkSemaphore));

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    for(uint32_t i = 0; i < state->imageCount; i++){
        assert(vkCreateSemaphore(state->device, &semaphoreCreateInfo, NULL, &state->renderFinishedSemaphores[i]) == VK_SUCCESS);
    }
}

void createSwapChain(struct VkState* state){
    printf("[+] Creating Swapchain\n");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &state->surfaceInfo.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, formats);
    for(int i = 0; i < formatCount; i++){
        if((formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)){
            printf("[!] Found desired format!\n");
            state->surfaceInfo.surfaceFormat = formats[i];
            break;
        }
    }
    printf("[+] Using Surface format %i with colorspace %i\n", state->surfaceInfo.surfaceFormat.format, state->surfaceInfo.surfaceFormat.colorSpace);

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentCount, NULL);
    VkPresentModeKHR presentModes[presentCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentCount, presentModes);
    for(int i = 0; i < presentCount; i++){
        if(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR){
            state->surfaceInfo.presentMode = presentModes[i];
            break;
        }
    }
    printf("[+] Using surface present mode %i\n", state->surfaceInfo.presentMode);

    if(state->surfaceInfo.capabilities.currentExtent.height != UINT32_MAX){
        state->extent = state->surfaceInfo.capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(state->window, &width, &height);
        state->extent.width = (uint32_t)(width);
        state->extent.height = (uint32_t)(height);
        state->extent.width = clamp(state->extent.width, 
                state->surfaceInfo.capabilities.minImageExtent.width, state->surfaceInfo.capabilities.maxImageExtent.width);
        state->extent.height = clamp(state->extent.height, 
                state->surfaceInfo.capabilities.minImageExtent.height, state->surfaceInfo.capabilities.maxImageExtent.height);
    }
    state->imageCount = state->surfaceInfo.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .minImageCount = state->surfaceInfo.capabilities.minImageCount,
        .imageFormat = state->surfaceInfo.surfaceFormat.format,
        .imageColorSpace = state->surfaceInfo.surfaceFormat.colorSpace,
        .imageExtent = state->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = state->queueFamilyIndices.graphicsFamily,
        .pQueueFamilyIndices = NULL,
        .preTransform = state->surfaceInfo.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = state->surfaceInfo.presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    assert(vkCreateSwapchainKHR(state->device, &createInfo, NULL, &state->swapchain) == VK_SUCCESS);

    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->imageCount, NULL);
    state->swapchainImages = malloc(sizeof(VkImage) * state->imageCount);
    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->imageCount, state->swapchainImages);

    createRenderFinishedSemaphores(state);
}

void createImageViews(struct VkState* state){
    printf("[+] Creating Image Views\n");
    state->swapchainImageViews = malloc(sizeof(VkImageView) * state->imageCount);
    for(int i = 0; i < state->imageCount; i++){
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = state->swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = state->surfaceInfo.surfaceFormat.format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,

        };
        assert(vkCreateImageView(state->device, &createInfo, NULL, &state->swapchainImageViews[i]) == VK_SUCCESS);
    }
}

void createRenderPass(struct VkState* state){
    printf("[+] Creating Render Pass\n");
    VkAttachmentDescription colorAttachment = {
        .format = state->surfaceInfo.surfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    assert(vkCreateRenderPass(state->device, &renderPassInfo, NULL, &state->renderPass) == VK_SUCCESS);
}

unsigned char * readFile(const char* filename, size_t* outSize){
    FILE* file = fopen(filename, "rb"); 
    if(!file) return NULL;

    if(fseek(file, 0, SEEK_END) != 0){ fclose(file); return NULL; }

    long fileSize = ftell(file);
    if(fileSize < 0) {fclose(file); return NULL;}

    rewind(file);

    unsigned char* buffer = (unsigned char*)malloc(fileSize);

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if(bytesRead != (size_t)fileSize){ free(buffer); fclose(file); return NULL; }

    fclose(file);
    if(outSize) *outSize = bytesRead;
    return buffer;
}

VkShaderModule createShaderModule(unsigned char * code, size_t size, struct VkState* state){
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const uint32_t*)code
    };
    VkShaderModule shader;
    assert(vkCreateShaderModule(state->device, &createInfo, NULL, &shader) == VK_SUCCESS);
    return shader;
}

void createShaders(struct VkState* state){
    printf("[+] Creating Shaders\n");
    size_t size;
    unsigned char * vertexShaderSPV = readFile("assets/vert.spv", &size);
    printf("[+] read size %lu\n",size);
    state->vertexShader = createShaderModule(vertexShaderSPV, size, state);

    unsigned char * fragmentShaderSPV = readFile("assets/frag.spv", &size);
    printf("[+] read size %lu\n",size);
    state->fragmentShader = createShaderModule(fragmentShaderSPV, size, state);

    free(vertexShaderSPV);
    free(fragmentShaderSPV);
}

void createGraphicsPipeline(struct VkState* state){
    printf("[+] Creating Graphics pipeline\n");
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = state->vertexShader,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = state->fragmentShader,
        .pName = "main",
        .pSpecializationInfo = NULL
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants[0] = 0.0f,
        .blendConstants[1] = 0.0f,
        .blendConstants[2] = 0.0f,
        .blendConstants[3] = 0.0f,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    assert(vkCreatePipelineLayout(state->device, &pipelineLayoutCreateInfo, NULL, &state->pipelineLayout) == VK_SUCCESS);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = NULL,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = state->pipelineLayout,
        .renderPass = state->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    assert(vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &state->graphicsPipeline) == VK_SUCCESS);
}

void createFrameBuffers(struct VkState* state){
    printf("[+] Creating Framebuffers\n");
    state->MAX_FRAMES_IN_FLIGHT = 2;
    state->swapchainFramebuffers = malloc(state->imageCount * sizeof(VkFramebuffer));
    for(size_t i = 0; i < state->imageCount; i++){
        VkImageView attachments[] = {
            state->swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = state->renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = state->extent.width,
            .height = state->extent.height,
            .layers = 1
        };
        assert(vkCreateFramebuffer(state->device, &framebufferInfo, NULL, &state->swapchainFramebuffers[i]) == VK_SUCCESS);
    }
}

void createCommandPool(struct VkState* state){
    printf("[+] Creating Command Pool\n");
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = state->queueFamilyIndices.graphicsFamily
    };
    assert(vkCreateCommandPool(state->device, &poolInfo, NULL, &state->commandPool) == VK_SUCCESS);
}

void createCommandBuffers(struct VkState* state){
    printf("[+] Creating Command Buffers\n");
    state->commandBuffers = malloc(state->MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = state->MAX_FRAMES_IN_FLIGHT
    };

    assert(vkAllocateCommandBuffers(state->device, &allocInfo, state->commandBuffers) == VK_SUCCESS);
}

void recordCommandBuffer(struct VkState* state, VkCommandBuffer commandBuffer, uint32_t imageIndex){
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = NULL
    };
    assert(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->renderPass,
        .framebuffer = state->swapchainFramebuffers[imageIndex],
        .renderArea.offset = {0, 0},
        .renderArea.extent = state->extent,
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->graphicsPipeline);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = state->extent.width,
        .height = state->extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = state->extent,
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
}

void createSyncObjects(struct VkState* state){
    printf("Creating Synchronization Objects\n");
    state->currentFrame = 0;
    state->imageAvailableSemaphores = malloc(state->MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    state->inFlightFences =  malloc(state->MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));

    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    for(int i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++){
        assert(vkCreateSemaphore(state->device, &semaphoreCreateInfo, NULL, &state->imageAvailableSemaphores[i]) == VK_SUCCESS);
        assert(vkCreateFence(state->device, &fenceCreateInfo, NULL, &state->inFlightFences[i]) == VK_SUCCESS);
    }
}

void cleanupSwapchain(struct VkState* state){
    for(int i = 0; i < state->imageCount; i++){ vkDestroyFramebuffer(state->device, state->swapchainFramebuffers[i], NULL); }
    for(int i = 0; i < state->imageCount; i++){ vkDestroyImageView(state->device, state->swapchainImageViews[i], NULL); }
    vkDestroySwapchainKHR(state->device, state->swapchain, NULL);
    destroyRenderFinishedSemaphores(state);
    free(state->swapchainFramebuffers); state->swapchainFramebuffers = NULL;
    free(state->swapchainImageViews); state->swapchainImageViews = NULL;
}

void recreateSwapChain(struct VkState* state){
    int width = 0, height = 0;
    glfwGetFramebufferSize(state->window, &width, &height);
    while(width == 0 || height == 0){
        glfwGetFramebufferSize(state->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(state->device);
    cleanupSwapchain(state);

    createSwapChain(state);
    createImageViews(state);
    createFrameBuffers(state);
}

void drawFrame(struct VkState* state){
    vkWaitForFences(state->device, 1, &state->inFlightFences[state->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = 
        vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX, state->imageAvailableSemaphores[state->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR){ recreateSwapChain(state); return;
    } else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)){ assert(0); }

    vkResetFences(state->device, 1, &state->inFlightFences[state->currentFrame]);
    vkResetCommandBuffer(state->commandBuffers[state->currentFrame], 0);
    recordCommandBuffer(state, state->commandBuffers[state->currentFrame], imageIndex);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &state->imageAvailableSemaphores[state->currentFrame],
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &state->commandBuffers[state->currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &state->renderFinishedSemaphores[imageIndex]
    };

    assert(vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, state->inFlightFences[state->currentFrame]) == VK_SUCCESS);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &state->renderFinishedSemaphores[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &state->swapchain,
        .pImageIndices = &imageIndex,
        .pResults = NULL
    };

    result = vkQueuePresentKHR(state->graphicsQueue, &presentInfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || state->framebufferResized){
        state->framebufferResized = false; recreateSwapChain(state);
    } else if (result != VK_SUCCESS){ assert(0); }
    state->currentFrame = (state->currentFrame + 1) % state->MAX_FRAMES_IN_FLIGHT;
};

void renderLoop(struct VkState* state){
    printf("[+] Entering Render Loop\n");
    while(!glfwWindowShouldClose(state->window)){
        glfwPollEvents();
        drawFrame(state);
    }
    vkDeviceWaitIdle(state->device);
}

void initGui(struct VkState* state){
    igCreateContext(NULL);
    ImGuiIO * io = igGetIO_Nil();
    printf("%s\n", io->IniFilename);
    ImGui_ImplGlfw_InitForVulkan(state->window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {
        .ApiVersion = VK_MAKE_VERSION(1, 0, 0),
        .Instance = state->instance,
        .PhysicalDevice = state->physicalDevice,
        .Device = state->device,
        .Queue = state->graphicsQueue,
        .QueueFamily = state->queueFamilyIndices.graphicsFamily,
        // etc ....
    };
    //ImGui_ImplVulkan_Init(&initInfo);
}

int main(void){
    printf("[+] Running Vulkan Program\n");
    struct VkState state;
    memset(&state, 0, sizeof(state));

    initWindow(&state);
    createInstance(&state);
    setupDebugMessenger(&state);
    pickPhysicalDevice(&state);
    setupQueues(&state);
    createLogicalDevice(&state);
    retrieveQueues(&state);
    createSurface(&state);
    createSwapChain(&state);
    createImageViews(&state);
    createRenderPass(&state);
    createShaders(&state);
    createGraphicsPipeline(&state);
    createFrameBuffers(&state);
    createCommandPool(&state);
    createCommandBuffers(&state);
    createSyncObjects(&state);

    initGui(&state);

    renderLoop(&state);
    cleanup(&state);
}

void cleanup(struct VkState* state){
    printf("[!] Cleaning up Instance\n");
    cleanupSwapchain(state);
    vkDestroyPipeline(state->device, state->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(state->device, state->pipelineLayout, NULL);
    vkDestroyRenderPass(state->device, state->renderPass, NULL);

    if(state->imageAvailableSemaphores != NULL){
        for(uint32_t i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++){
            if(state->imageAvailableSemaphores[i] != VK_NULL_HANDLE){
                vkDestroySemaphore(state->device, state->imageAvailableSemaphores[i], NULL);
            }
        }
    }

    if(state->inFlightFences != NULL){
        for(uint32_t i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++){
            if(state->inFlightFences[i] != VK_NULL_HANDLE){
                vkDestroyFence(state->device, state->inFlightFences[i], NULL);
            }
        }
    }

    vkDestroyShaderModule(state->device, state->vertexShader, NULL);
    vkDestroyShaderModule(state->device, state->fragmentShader, NULL);

    vkDestroyCommandPool(state->device, state->commandPool, NULL);
    vkDestroyDevice(state->device, NULL);

    DestroyDebugUtilsMessengerEXT(state->instance, state->debugMessenger, NULL);

    vkDestroySurfaceKHR(state->instance, state->surface, NULL);
    vkDestroyInstance(state->instance, NULL);

    glfwDestroyWindow(state->window);
    free(state->swapchainImages); state->swapchainImages = NULL;
    free(state->swapchainImageViews); state->swapchainImageViews = NULL;
    free(state->swapchainFramebuffers); state->swapchainFramebuffers = NULL;
    free(state->commandBuffers); state->commandBuffers = NULL;
    free(state->imageAvailableSemaphores); state->imageAvailableSemaphores = NULL;
    free(state->inFlightFences); state->inFlightFences = NULL;

    glfwTerminate();
}
