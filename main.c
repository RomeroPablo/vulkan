// main.c
#include <cglm/util.h>
#include <math.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_VULKAN
#define CIMGUI_USE_GLFW
#include "external/cimgui.h"
#include "external/cimgui_impl.h"
#include "external/ui.h"
#include "external/stb_image.h"
#include "external/tinyobj_loader_c.h"
#include "external/hash.h"
#include "assets/berkeley.h"

struct Vertex {
    vec3 pos;
    vec3 color;
    vec2 texCoord;
};

struct UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

struct ObjectState {
    struct Vertex* vertices;
    uint32_t vertexCount;
    uint32_t* indices;
    uint32_t indexCount;

    VkVertexInputBindingDescription bindingDescription;
    VkVertexInputAttributeDescription* attributeDescriptions;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkBuffer* uniformBuffers;
    VkDeviceMemory* uniformBuffersMemory;
    void** uniformBuffersMapped;

    VkDescriptorSet* descriptorSets;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    VkImage textureImage;
    VkImageView textureImageView;
    VkDeviceMemory textureImageMemory;
    VkSampler textureSampler;

    VkImage depthImage;
    VkImageView depthImageView;
    VkDeviceMemory depthImageMemory;

    uint32_t mipLevels;
};

struct ImguiState{
    VkDescriptorPool descriptorPool;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
};

struct Camera{
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    float yaw;
    float pitch;
    float moveSpeed;
    float mouseSensitivity;
    bool firstMouse;
    double lastX;
    double lastY;
    bool mouseCaptured;
};

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
    VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
    VkQueueFamilyProperties* queueFamilyProperties;
    uint32_t queueFamilyCount;
    float queuePriority;
    struct QueueFamilyIndices{
        uint32_t graphicsFamily;
        uint32_t computeFamily;
    }queueFamilyIndices;
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
    VkCommandPool transientPool;
    VkCommandPool commandPool;
    VkCommandBuffer* commandBuffers;
    VkSemaphore* imageAvailableSemaphores;
    VkSemaphore* renderFinishedSemaphores;
    VkFence* inFlightFences;
    uint32_t MAX_FRAMES_IN_FLIGHT;
    uint32_t currentFrame;
    bool framebufferResized;
    double lastFrameTime;
    float deltaTime;
    VkSampleCountFlagBits msaaSamples;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    struct Camera camera;
    struct ObjectState objectState;
    struct ImguiState imguiState;

    VkDebugUtilsMessengerEXT debugMessenger;
}typedef VkState;

void cleanup(VkState* state);

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

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT* createInfo){
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo->messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debugCallback;
    createInfo->pUserData = NULL;
}

void setupDebugMessenger(VkState* state){
    printf("[+] Setting up Debug Messenger\n");
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
    populateDebugMessengerCreateInfo(&createInfo);
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
    VkState* temp = (VkState*)glfwGetWindowUserPointer(window);
    temp->framebufferResized = true;
}

void initWindow(VkState* state){
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

void initCamera(VkState* state){
    glm_vec3_copy((vec3){0.0f, -3.0f, 1.5f}, state->camera.position);
    state->camera.yaw = 90.0f;
    state->camera.pitch = -15.0f;
    state->camera.moveSpeed = 3.0f;
    state->camera.mouseSensitivity = 0.1f;
    state->camera.firstMouse = true;
    state->camera.mouseCaptured = false;

    vec3 front = {
        cosf(glm_rad(state->camera.pitch)) * cosf(glm_rad(state->camera.yaw)),
        cosf(glm_rad(state->camera.pitch)) * sinf(glm_rad(state->camera.yaw)),
        sinf(glm_rad(state->camera.pitch)),
    };
    glm_vec3_normalize_to(front, state->camera.front);
    vec3 worldUp = {0.0f, 0.0f, 1.0f};
    glm_vec3_crossn(state->camera.front, worldUp, state->camera.right);
    glm_vec3_crossn(state->camera.right, state->camera.front, state->camera.up);
}

void createInstance(VkState* state){
    printf("[+] Creating Vulkan Instance\n");
    bool val = checkValidationLayerSupport();

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
    if (!glfwExtensions || glfwExtensionsCount == 0) {
        fprintf(stderr, "GLFW did not report required instance extensions\n");
    }
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
        .enabledLayerCount = val ? validationLayerCount : 0,
        .ppEnabledLayerNames = val ? validationLayers : NULL,
    };
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if(val){
        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = (const void*)&debugCreateInfo;
    }

    VkResult result = vkCreateInstance(&createInfo, NULL, &state->instance);
    assert(result == VK_SUCCESS);
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDeviceProperties* props){
    VkSampleCountFlags counts = props->limits.framebufferColorSampleCounts & 
                                props->limits.framebufferDepthSampleCounts;
    if(counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if(counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if(counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if(counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if(counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if(counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

void pickPhysicalDevice(VkState* state){
    printf("[+] Picking Physical Device\n");
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(state->instance, &deviceCount, NULL);
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(state->instance, &deviceCount, devices);
    state->physicalDevice = devices[0];

    vkGetPhysicalDeviceProperties(state->physicalDevice, &state->physicalDeviceProperties);
    printf("[+] Using Device %s\n", state->physicalDeviceProperties.deviceName);

    vkGetPhysicalDeviceFeatures(state->physicalDevice, &state->physicalDeviceFeatures);

    vkGetPhysicalDeviceMemoryProperties(state->physicalDevice, &state->physicalMemoryProperties);

    state->msaaSamples = getMaxUsableSampleCount(&state->physicalDeviceProperties);

    free(devices);
}

void setupQueues(VkState* state){
    printf("[+] Setting up Queues\n");
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &state->queueFamilyCount, NULL);
    state->queueFamilyProperties = malloc(sizeof(VkQueueFamilyProperties) * state->queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &state->queueFamilyCount, state->queueFamilyProperties);
    for(int i = 0; i < state->queueFamilyCount; i++){
        if((state->queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)){
            if(state->queueFamilyProperties[i].queueCount > state->queueCount.graphics){
                state->queueFamilyIndices.graphicsFamily = i;
                state->queueCount.graphics = state->queueFamilyProperties[i].queueCount;
            }
        }
        if((state->queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)){
            if(state->queueFamilyProperties[i].queueCount > state->queueCount.compute){
                state->queueFamilyIndices.computeFamily = i;
                state->queueCount.compute = state->queueFamilyProperties[i].queueCount;
            }
        }
    }
    printf("[+] Graphics using Queue Family [ %i ] with [ %i ] queues\n", state->queueFamilyIndices.graphicsFamily, state->queueCount.graphics);
    printf("[+] Compute using Queue Family  [ %i ] with [ %i ] queues\n", state->queueFamilyIndices.computeFamily, state->queueCount.compute);
}

void createLogicalDevice(VkState* state){
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

    VkPhysicalDeviceFeatures enabledFeatures = {0};
    enabledFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = queueCreateInfoCount,
        .ppEnabledExtensionNames = enabledExtensions,
        .enabledExtensionCount = enabledExtensionCount,
        .pEnabledFeatures = &enabledFeatures,
        .ppEnabledLayerNames = NULL,
        .enabledLayerCount = 0,
    };
    assert(vkCreateDevice(state->physicalDevice, &createInfo, NULL, &state->device) == VK_SUCCESS);
}

void retrieveQueues(VkState* state){
    printf("[+] Retrieving Queues\n");
    vkGetDeviceQueue(state->device, state->queueFamilyIndices.graphicsFamily, 0, &state->graphicsQueue);
    vkGetDeviceQueue(state->device, state->queueFamilyIndices.computeFamily, 0, &state->computeQueue);
}

void createSurface(VkState* state){
    printf("[+] Creating Surface\n");
    VkResult result = glfwCreateWindowSurface(state->instance, state->window, NULL, &state->surface);
    if(result != VK_SUCCESS) {
        fprintf(stderr, "glfwCreateWindowSurface failed: %d\n", result);
        return;
    }
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(state->physicalDevice, state->queueFamilyIndices.graphicsFamily, state->surface, &presentSupport);
    assert(presentSupport);
};

static inline int clamp(int input, int min, int max){
    return input < min ? min : (input > max ? max : input);
}

static void destroyRenderFinishedSemaphores(VkState* state){
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

static void createRenderFinishedSemaphores(VkState* state){
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

void createSwapChain(VkState* state){
    printf("[+] Creating Swapchain\n");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &state->surfaceInfo.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, formats);
    printf("[!] Total Formats : %i \n", formatCount);
    for(int i = 0; i < formatCount; i++){
        printf("[!] Found Format %i With Color Space %i \n", formats[i].format, formats[i].colorSpace);
        if((formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) && (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)){
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
    printf("[+] Using %i swapchain images\n", state->imageCount);

    createRenderFinishedSemaphores(state);
}

VkImageView createImageView(VkState* state, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels){
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = aspectFlags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = mipLevels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    VkImageView imageView;
    assert(vkCreateImageView(state->device, &viewInfo, NULL, &imageView) == VK_SUCCESS);
    return imageView;
}

void createImageViews(VkState* state){
    printf("[+] Creating Image Views\n");
    state->swapchainImageViews = malloc(sizeof(VkImageView) * state->imageCount);
    for(int i = 0; i < state->imageCount; i++){
        state->swapchainImageViews[i] = 
            createImageView(state, state->swapchainImages[i], state->surfaceInfo.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkFormat findSupportedFormat(VkState* state, VkFormat* candidates, uint32_t count, VkImageTiling tiling, VkFormatFeatureFlags features){
    for(int i = 0; i < count; i++){
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(state->physicalDevice, candidates[i], &props);
        if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features)) return candidates[i];
        if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return candidates[i];
    }
    return -1;
}

VkFormat findDepthFormat(VkState* state){
    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT};
    return findSupportedFormat(state, formats, sizeof(formats)/sizeof(VkFormat), 
            VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void createRenderPass(VkState* state){
    printf("[+] Creating Render Pass\n");
    VkAttachmentDescription colorAttachment = {
        .format = state->surfaceInfo.surfaceFormat.format,
        .samples = state->msaaSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription depthAttachment = {
        .format = findDepthFormat(state),
        .samples = state->msaaSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription colorAttachmentResolve = {
        .format = state->surfaceInfo.surfaceFormat.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference colorAttachmentResolveRef = {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .pResolveAttachments = &colorAttachmentResolveRef,
    };

    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment, colorAttachmentResolve};

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof(attachments)/sizeof(VkAttachmentDescription),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    assert(vkCreateRenderPass(state->device, &renderPassInfo, NULL, &state->renderPass) == VK_SUCCESS);
}

static void tinyobj_file_reader(void *ctx, const char *filename, 
        int is_mtl, const char *obj_filename, char **out_buf, size_t *out_len){
    (void)ctx; (void)is_mtl;
    (void)obj_filename;

    FILE *f = fopen(filename, "rb");
    if (!f) { *out_buf = NULL; *out_len = 0; return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc((size_t)size + 1);
    if (!buffer) { fclose(f); *out_buf = NULL; *out_len = 0; return; }
    if (fread(buffer, 1, (size_t)size, f) != (size_t)size){ 
        fclose(f);
        free(buffer); *out_buf = NULL;
        *out_len = 0; return;
    }
    buffer[size] = '\0';
    fclose(f);

    *out_buf = buffer;
    *out_len = (size_t)size;
}

void loadModel(VkState* state){
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = NULL;
    tinyobj_material_t* materials = NULL;
    size_t num_shapes = 0;
    size_t num_materials = 0;

    tinyobj_attrib_init(&attrib);
    int ret = tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials,
            "assets/viking.obj", tinyobj_file_reader, NULL, TINYOBJ_FLAG_TRIANGULATE);
    if(ret != TINYOBJ_SUCCESS){
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(shapes, num_shapes);
        tinyobj_materials_free(materials, num_materials);
        assert(0);
    }

    size_t vertex_count = attrib.num_faces;
    state->objectState.vertexCount = (uint32_t)vertex_count;
    state->objectState.indexCount = (uint32_t)vertex_count;
    state->objectState.vertices = malloc(sizeof(struct Vertex) * vertex_count);
    state->objectState.indices = malloc(sizeof(uint32_t) * vertex_count);

    for(size_t i = 0; i < vertex_count; ++i){
        tinyobj_vertex_index_t idx = attrib.faces[i];
        struct Vertex vertex = {0};

        if(idx.v_idx != TINYOBJ_INVALID_INDEX){
            size_t base = (size_t)idx.v_idx * 3;
            vertex.pos[0] = attrib.vertices[base + 0];
            vertex.pos[1] = attrib.vertices[base + 1];
            vertex.pos[2] = attrib.vertices[base + 2];
        }

        if(idx.vt_idx != TINYOBJ_INVALID_INDEX){
            size_t base = (size_t)idx.vt_idx * 2;
            vertex.texCoord[0] = attrib.texcoords[base + 0];
            vertex.texCoord[1] = 1.0f - attrib.texcoords[base + 1];
        }

        vertex.color[0] = 1.0f;
        vertex.color[1] = 1.0f;
        vertex.color[2] = 1.0f;

        state->objectState.vertices[i] = vertex;
        state->objectState.indices[i] = (uint32_t)i;
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
}

void initObjectState(VkState* state){
    printf("[+] Creating Object State\n");
    loadModel(state);
/*
    struct Vertex vertTemp[] = {
        {{-0.5f, -0.5f,  0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    };
    state->objectState.vertexCount = sizeof(vertTemp)/sizeof(struct Vertex);
    state->objectState.vertices = malloc(sizeof(struct Vertex) * state->objectState.vertexCount);
    memcpy(state->objectState.vertices, vertTemp, sizeof(vertTemp));

    uint32_t indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
    };
    state->objectState.indexCount = sizeof(indices)/sizeof(uint32_t);
    state->objectState.indices = malloc(sizeof(state->objectState.indices) * state->objectState.indexCount);
    memcpy(state->objectState.indices, indices, sizeof(indices));
*/

    printf("[+] Model Loaded\n");
    printf("[+] Found %i Vertices and %i Indices\n", state->objectState.vertexCount, state->objectState.indexCount);
    state->objectState.bindingDescription.binding = 0;
    state->objectState.bindingDescription.stride = sizeof(struct Vertex);
    state->objectState.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    state->objectState.attributeDescriptions = malloc(3*sizeof(VkVertexInputAttributeDescription));
    state->objectState.attributeDescriptions[0].binding = 0;
    state->objectState.attributeDescriptions[0].location = 0;
    state->objectState.attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    state->objectState.attributeDescriptions[0].offset = offsetof(struct Vertex, pos);

    state->objectState.attributeDescriptions[1].binding = 0;
    state->objectState.attributeDescriptions[1].location = 1;
    state->objectState.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    state->objectState.attributeDescriptions[1].offset = offsetof(struct Vertex, color);

    state->objectState.attributeDescriptions[2].binding = 0;
    state->objectState.attributeDescriptions[2].location = 2;
    state->objectState.attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    state->objectState.attributeDescriptions[2].offset = offsetof(struct Vertex, texCoord);
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

VkShaderModule createShaderModule(unsigned char * code, size_t size, VkState* state){
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = (const uint32_t*)code
    };
    VkShaderModule shader;
    assert(vkCreateShaderModule(state->device, &createInfo, NULL, &shader) == VK_SUCCESS);
    return shader;
}

void createShaders(VkState* state){
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

void createDescriptorSetLayout(VkState* state){
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    };

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = NULL,
    };

    VkDescriptorSetLayoutBinding layoutBindings[] = { uboLayoutBinding, samplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof(layoutBindings)/sizeof(VkDescriptorSetLayoutBinding),
        .pBindings = layoutBindings,
    };

    assert(vkCreateDescriptorSetLayout(state->device, &layoutInfo, NULL, &state->objectState.descriptorSetLayout) == VK_SUCCESS);
}

void createGraphicsPipeline(VkState* state){
    printf("[+] Creating Graphics pipeline\n");

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &state->objectState.bindingDescription,

        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = state->objectState.attributeDescriptions 
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
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = state->msaaSamples,
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
    
    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state->objectState.descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL,
    };

    assert(vkCreatePipelineLayout(state->device, &pipelineLayoutCreateInfo, NULL, &state->pipelineLayout) == VK_SUCCESS);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pDynamicState = &dynamicState,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .layout = state->pipelineLayout,
        .renderPass = state->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    assert(vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &state->graphicsPipeline) == VK_SUCCESS);
}

void createFrameBuffers(VkState* state){
    printf("[+] Creating Framebuffers\n");
    state->MAX_FRAMES_IN_FLIGHT = (2 > state->imageCount ) ? 2 : state->imageCount;
    printf("[+] Max Frames in Flight %i\n", state->imageCount);
    state->swapchainFramebuffers = malloc(state->imageCount * sizeof(VkFramebuffer));
    for(size_t i = 0; i < state->imageCount; i++){
        VkImageView attachments[] = {
            state->colorImageView,
            state->objectState.depthImageView,
            state->swapchainImageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = state->renderPass,
            .attachmentCount = sizeof(attachments)/sizeof(VkImageView),
            .pAttachments = attachments,
            .width = state->extent.width,
            .height = state->extent.height,
            .layers = 1,
        };
        assert(vkCreateFramebuffer(state->device, &framebufferInfo, NULL, &state->swapchainFramebuffers[i]) == VK_SUCCESS);
    }
}

void createCommandPool(VkState* state){
    printf("[+] Creating Command Pool\n");
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = state->queueFamilyIndices.graphicsFamily
    };
    assert(vkCreateCommandPool(state->device, &poolInfo, NULL, &state->commandPool) == VK_SUCCESS);

    VkCommandPoolCreateInfo transientInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags =  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = state->queueFamilyIndices.graphicsFamily
    };
    assert(vkCreateCommandPool(state->device, &transientInfo, NULL, &state->transientPool) == VK_SUCCESS);
}

uint32_t findMemoryType(VkState* state, uint32_t typeFilter, VkMemoryPropertyFlags properties){
    for(uint32_t i = 0; i < state->physicalMemoryProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i))&&
        (state->physicalMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return UINT32_MAX;
}

VkCommandBuffer beginSingleTimeCommands(VkState* state){
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, 
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->transientPool,
        .commandBufferCount = 1,
    };
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(state->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void endSingleTimeCommands(VkState* state, VkCommandBuffer commandBuffer){
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };
    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->graphicsQueue);
    vkFreeCommandBuffers(state->device, state->transientPool, 1, &commandBuffer);
}

void createBuffer(VkState* state, 
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer* buffer,
        VkDeviceMemory* bufferMemory){

    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .flags = 0,
    };

    assert(vkCreateBuffer(state->device, &bufferInfo, NULL, buffer) == VK_SUCCESS);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(state->device, *buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(state, memRequirements.memoryTypeBits, properties)
    };

    assert(vkAllocateMemory(state->device, &allocInfo, NULL, bufferMemory) == VK_SUCCESS);
    vkBindBufferMemory(state->device, *buffer, *bufferMemory, 0);
}

void copyBuffer(VkState* state, VkBuffer src, VkBuffer dst, VkDeviceSize size){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(state);

    VkBufferCopy copyRegion = { .size = size };
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    endSingleTimeCommands(state, commandBuffer);
}

void copyBuffertoImage(VkState* state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(state);
    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {0, 0, 0},
        .imageExtent = { width, height, 1 }
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(state, commandBuffer);
};

bool hasStencilComponent(VkFormat format){
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(VkState* state, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels){
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(state);
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = mipLevels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(hasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }else{ return; }

    vkCmdPipelineBarrier(commandBuffer, 
            sourceStage, destinationStage, 
            0, 
            0, NULL, 
            0, NULL, 
            1, &barrier);

    endSingleTimeCommands(state, commandBuffer);
};

void createImage(VkState* state, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
        VkImage* image, VkDeviceMemory* imageMemory){
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .format = format,
        .tiling = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = numSamples,
        .flags = 0,
    };
    assert(vkCreateImage(state->device, &imageInfo, NULL, image) == VK_SUCCESS);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(state->device, *image, &memReqs);
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = findMemoryType(state, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    assert(vkAllocateMemory(state->device, &allocInfo, NULL, imageMemory) == VK_SUCCESS);
    vkBindImageMemory(state->device, *image, *imageMemory, 0);
}

void createColorResource(VkState* state){
    VkFormat colorFormat = state->surfaceInfo.surfaceFormat.format;
    createImage(state, state->surfaceInfo.capabilities.currentExtent.width, state->surfaceInfo.capabilities.currentExtent.height, 1,
            state->msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &state->colorImage, &state->colorImageMemory);
    state->colorImageView = createImageView(state, state->colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void generateMipmaps(VkState* state, VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels){
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(state->physicalDevice, imageFormat, &formatProps);
    if(!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)){
        printf("[!] Image does not support linear blitting!");
        return;
    }
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(state);
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .subresourceRange.levelCount = 1,
    };
    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;
    for(uint32_t i = 1; i < mipLevels; i++){
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 
                            0, 0, NULL, 0, NULL, 1, &barrier);
        VkImageBlit blit = {
            .srcOffsets[0] = { 0, 0, 0, },
            .srcOffsets[1] = { mipWidth, mipHeight, 1},
            .srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .srcSubresource.mipLevel = i - 1,
            .srcSubresource.baseArrayLayer = 0,
            .srcSubresource.layerCount = 1,
            .dstOffsets[0] = {0, 0, 0},
            .dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, 
                               mipHeight > 1 ? mipHeight / 2 : 1, 
                               1 },
            .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .dstSubresource.mipLevel = i,
            .dstSubresource.baseArrayLayer = 0,
            .dstSubresource.layerCount = 1,
        };
        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                0, 0, NULL, 0, NULL, 1, &barrier);

        if(mipWidth > 1) mipWidth /= 2;
        if(mipHeight > 1) mipHeight /= 2;
    }
    barrier.subresourceRange.baseMipLevel = mipLevels  - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
                            0, 0, NULL, 0, NULL, 1, &barrier);

    endSingleTimeCommands(state, commandBuffer);
}

void createTextureImage(VkState* state){
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("assets/viking.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if(!pixels){
        printf("[!] Failed to load image\n");
    }
    printf("[+] Loaded %i x %i image\n", texWidth, texHeight);

    state->objectState.mipLevels = floor(log2(glm_max(texWidth, texHeight))) + 1;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT 
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            &stagingBuffer, &stagingMemory);
    void* data;
    vkMapMemory(state->device, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(state->device, stagingMemory);
    stbi_image_free(pixels);

    createImage(state, texWidth, texHeight, state->objectState.mipLevels, VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_UNORM, 
            VK_IMAGE_TILING_OPTIMAL, 
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            &state->objectState.textureImage, &state->objectState.textureImageMemory);

    transitionImageLayout(state, state->objectState.textureImage, 
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, state->objectState.mipLevels);
    copyBuffertoImage(state, stagingBuffer, state->objectState.textureImage, texWidth, texHeight);
    generateMipmaps(state, state->objectState.textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, state->objectState.mipLevels);

//    transitionImageLayout(state, state->objectState.textureImage, 
//            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, state->objectState.mipLevels);

    vkDestroyBuffer(state->device, stagingBuffer, NULL);
    vkFreeMemory(state->device, stagingMemory, NULL);
}

void createTextureImageView(VkState* state){
    state->objectState.textureImageView = createImageView(state, state->objectState.textureImage, 
            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, state->objectState.mipLevels);
}

void createTextureSampler(VkState* state){
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = state->physicalDeviceProperties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0,
        .maxLod = state->objectState.mipLevels,
    };
    assert(vkCreateSampler(state->device, &samplerInfo, NULL, &state->objectState.textureSampler) == VK_SUCCESS);

}

void createDepthResource(VkState* state){
    printf("[+] Creating Depth Resource\n");
    VkFormat depthFormat = findDepthFormat(state);
    createImage(state, state->surfaceInfo.capabilities.currentExtent.width, state->surfaceInfo.capabilities.currentExtent.height, 1,
            state->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            &state->objectState.depthImage, &state->objectState.depthImageMemory);
    state->objectState.depthImageView = createImageView(state, state->objectState.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

    transitionImageLayout(state, state->objectState.depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1); //mip levels have not yet been created
}

void createVertexBuffer(VkState* state){
    printf("[+] Creating Vertex Buffer\n");
    VkDeviceSize bufferSize = sizeof(struct Vertex) * state->objectState.vertexCount;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(state, 
            bufferSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            &stagingBuffer, 
            &stagingBufferMemory);

    void* data;
    vkMapMemory(state->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, state->objectState.vertices, bufferSize);
    vkUnmapMemory(state->device, stagingBufferMemory);

    createBuffer(state, 
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &state->objectState.vertexBuffer,
        &state->objectState.vertexBufferMemory
        );

    copyBuffer(state, stagingBuffer, state->objectState.vertexBuffer, bufferSize);

    vkDestroyBuffer(state->device, stagingBuffer, NULL);
    vkFreeMemory(state->device, stagingBufferMemory, NULL);
}

void createIndexBuffer(VkState* state){
    printf("[+] Creating Index Buffer\n");
    VkDeviceSize bufferSize = sizeof(*state->objectState.indices) * state->objectState.indexCount;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(state, 
            bufferSize, 
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            &stagingBuffer, 
            &stagingBufferMemory);

    void* data;
    vkMapMemory(state->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, state->objectState.indices, bufferSize);
    vkUnmapMemory(state->device, stagingBufferMemory);

    createBuffer(state, 
            bufferSize, 
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            &state->objectState.indexBuffer, 
            &state->objectState.indexBufferMemory);

    copyBuffer(state, stagingBuffer, state->objectState.indexBuffer, bufferSize);

    vkDestroyBuffer(state->device, stagingBuffer, NULL);
    vkFreeMemory(state->device, stagingBufferMemory, NULL);
}

void createUniformBuffers(VkState* state){
    VkDeviceSize bufferSize = sizeof(struct UniformBufferObject);
    state->objectState.uniformBuffers = malloc(sizeof(VkBuffer) * state->MAX_FRAMES_IN_FLIGHT);
    state->objectState.uniformBuffersMemory = malloc(sizeof(VkDeviceMemory) * state->MAX_FRAMES_IN_FLIGHT);
    state->objectState.uniformBuffersMapped = malloc(sizeof(void*) * state->MAX_FRAMES_IN_FLIGHT);

    for(size_t i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++){
        createBuffer(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                &state->objectState.uniformBuffers[i], &state->objectState.uniformBuffersMemory[i]);
        vkMapMemory(state->device, state->objectState.uniformBuffersMemory[i], 
                0, bufferSize, 0, &state->objectState.uniformBuffersMapped[i]);
    }
}

void createDescriptorPool(VkState* state){
    VkDescriptorPoolSize uniformPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = state->MAX_FRAMES_IN_FLIGHT
    };

    VkDescriptorPoolSize samplerPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = state->MAX_FRAMES_IN_FLIGHT
    };

    VkDescriptorPoolSize descriptorPoolSizes[] = { uniformPoolSize, samplerPoolSize };
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = sizeof(descriptorPoolSizes)/sizeof(VkDescriptorPoolSize),
        .pPoolSizes = descriptorPoolSizes,
        .maxSets = state->MAX_FRAMES_IN_FLIGHT,
        .flags = 0,
    };

    assert(vkCreateDescriptorPool(state->device, &descriptorPoolCreateInfo, NULL, &state->objectState.descriptorPool) == VK_SUCCESS);
}

void createDescriptorSets(VkState* state){
    VkDescriptorSetLayout* layouts = malloc(sizeof(VkDescriptorSetLayout)*state->MAX_FRAMES_IN_FLIGHT);
    for(int i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++) layouts[i] = state->objectState.descriptorSetLayout;
    VkDescriptorSetAllocateInfo allocInfo ={
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state->objectState.descriptorPool,
        .descriptorSetCount = state->MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts,
    };

    state->objectState.descriptorSets = malloc(sizeof(VkDescriptorSet)*state->MAX_FRAMES_IN_FLIGHT);
    assert(vkAllocateDescriptorSets(state->device, &allocInfo, state->objectState.descriptorSets) == VK_SUCCESS);

    for(int i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++){
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = state->objectState.uniformBuffers[i],
            .offset = 0,
            .range = sizeof(struct UniformBufferObject),
        };
        VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = state->objectState.textureImageView,
            .sampler = state->objectState.textureSampler,
        };
        VkWriteDescriptorSet bufferDescriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->objectState.descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo,
        };
        VkWriteDescriptorSet imageDescriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->objectState.descriptorSets[i],
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &imageInfo,
        };
        VkWriteDescriptorSet descriptorWrite[] = { bufferDescriptorWrite, imageDescriptorWrite };
        vkUpdateDescriptorSets(state->device, sizeof(descriptorWrite)/sizeof(VkWriteDescriptorSet), descriptorWrite, 0, NULL);
    }
    free(layouts);
}

void createCommandBuffers(VkState* state){
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

void recordCommandBuffer(VkState* state, VkCommandBuffer commandBuffer, uint32_t imageIndex, ImDrawData* texture){
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = NULL
    };
    assert(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS);

    VkClearValue clearColor[2] = {0};
    clearColor[0].color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
    clearColor[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0};
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->renderPass,
        .framebuffer = state->swapchainFramebuffers[imageIndex],
        .renderArea.offset = {0, 0},
        .renderArea.extent = state->extent,
        .clearValueCount = sizeof(clearColor)/sizeof(VkClearValue),
        .pClearValues = clearColor,
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

    VkBuffer vertexBuffers[] = {state->objectState.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &state->objectState.vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, state->objectState.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelineLayout, 
            0, 1, &state->objectState.descriptorSets[state->currentFrame], 0, NULL);

    vkCmdDrawIndexed(commandBuffer, state->objectState.indexCount, 1, 0, 0, 0);

    ImGui_ImplVulkan_RenderDrawData(texture, commandBuffer, VK_NULL_HANDLE);

    vkCmdEndRenderPass(commandBuffer);
    assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
}

void createSyncObjects(VkState* state){
    printf("[+] Creating Synchronization Objects\n");
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

void cleanupSwapchain(VkState* state){
    for(int i = 0; i < state->imageCount; i++){ vkDestroyFramebuffer(state->device, state->swapchainFramebuffers[i], NULL); }
    for(int i = 0; i < state->imageCount; i++){ vkDestroyImageView(state->device, state->swapchainImageViews[i], NULL); }
    vkDestroyImageView(state->device, state->objectState.depthImageView, NULL);
    vkDestroyImage(state->device, state->objectState.depthImage, NULL);
    vkFreeMemory(state->device, state->objectState.depthImageMemory, NULL);

    vkDestroyImageView(state->device, state->colorImageView, NULL);
    vkDestroyImage(state->device, state->colorImage, NULL);
    vkFreeMemory(state->device, state->colorImageMemory, NULL);

    vkDestroySwapchainKHR(state->device, state->swapchain, NULL);
    destroyRenderFinishedSemaphores(state);
    free(state->swapchainFramebuffers); state->swapchainFramebuffers = NULL;
    free(state->swapchainImageViews); state->swapchainImageViews = NULL;
}

void recreateSwapChain(VkState* state){
    int width = 0, height = 0;
    glfwGetFramebufferSize(state->window, &width, &height);
    while(width == 0 || height == 0){
        glfwGetFramebufferSize(state->window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(state->device);
    cleanupSwapchain(state);

    createSwapChain(state);
    ImGui_ImplVulkan_SetMinImageCount(state->MAX_FRAMES_IN_FLIGHT);
    createImageViews(state);
    createColorResource(state);
    createDepthResource(state);
    createFrameBuffers(state);
}

void initImGuiDescriptorPool(VkState* state){
    printf("[+] Creating ImGui Descriptor Pool\n");
    VkDescriptorPoolSize uniformPS = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = state->MAX_FRAMES_IN_FLIGHT,
    };

    VkDescriptorPoolSize imagePS = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = state->MAX_FRAMES_IN_FLIGHT,
    };

    VkDescriptorPoolSize samplerPS = {
        .type = VK_DESCRIPTOR_TYPE_SAMPLER,
        .descriptorCount = state->MAX_FRAMES_IN_FLIGHT
    };

    VkDescriptorPoolSize pS[] = {uniformPS, imagePS, samplerPS};
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = sizeof(pS)/sizeof(VkDescriptorPoolSize),
        .pPoolSizes = pS,
        .maxSets = state->MAX_FRAMES_IN_FLIGHT,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    };

    assert(vkCreateDescriptorPool(state->device, &descriptorPoolCreateInfo, NULL, &state->imguiState.descriptorPool) == VK_SUCCESS);
}

void uploadUIData(VkState* state){
    VkCommandBufferBeginInfo cbbi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &state->imguiState.commandBuffer
    };
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->transientPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    assert(vkAllocateCommandBuffers(state->device, &allocInfo, &state->imguiState.commandBuffer) == VK_SUCCESS);
    vkBeginCommandBuffer(state->imguiState.commandBuffer, &cbbi);
    ImGui_ImplVulkan_NewFrame(); // intial upload for e.g. font data ...
    vkEndCommandBuffer(state->imguiState.commandBuffer);
    vkQueueSubmit(state->graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->graphicsQueue);
    vkFreeCommandBuffers(state->device, state->transientPool, 1, &state->imguiState.commandBuffer);
}

void initGui(VkState* state){
    initImGuiDescriptorPool(state);
    igCreateContext(NULL);
    ImGuiIO * io = igGetIO_Nil();
    io->IniFilename = NULL;
    ImGui_ImplGlfw_InitForVulkan(state->window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {
        .ApiVersion = VK_MAKE_VERSION(1, 0, 0),
        .Instance = state->instance,
        .PhysicalDevice = state->physicalDevice,
        .Device = state->device,
        .QueueFamily = state->queueFamilyIndices.graphicsFamily,
        .Queue = state->graphicsQueue,
        .DescriptorPool = state->imguiState.descriptorPool,
        .ImageCount = state->imageCount,
        .MinImageCount = state->MAX_FRAMES_IN_FLIGHT,
        .MSAASamples = state->msaaSamples,
        .RenderPass = state->renderPass,
        .Subpass = 0,
    };
    ImGui_ImplVulkan_Init(&initInfo);
    ImGuiStyle* style = igGetStyle();
    ImFontConfig* fontConfig =  ImFontConfig_ImFontConfig();
    fontConfig->FontDataOwnedByAtlas = false;
    ImFont* berkeleyFont = ImFontAtlas_AddFontFromMemoryTTF(
            io->Fonts, (void*)Berkeley_ttf, 
            Berkeley_ttf_size, 18.0f, fontConfig, NULL);
    setEngineStyle(style);
    uploadUIData(state);
}

void updateUniformBuffer(VkState* state, uint32_t currentImage){
    static struct timespec start = {0};
    struct timespec current;

    if (start.tv_sec == 0 && start.tv_nsec == 0)
        clock_gettime(CLOCK_MONOTONIC, &start);

    clock_gettime(CLOCK_MONOTONIC, &current);

    double deltaTime =
        (current.tv_sec - start.tv_sec) +
        (current.tv_nsec - start.tv_nsec) / 1e9;

    struct UniformBufferObject ubo = {
        .model = {0},
        .view = {0},
        .proj = {0},
    };

    mat4 model; glm_mat4_identity(model);
    float angle = glm_rad(90.0f) * 1.0;//deltaTime;
    float* axis = (vec3){0.0f, 0.0f, 1.0f};
    glm_rotate(model, angle, axis);
    glm_mat4_copy(model, ubo.model);

    mat4 view;
    vec3 center;
    glm_vec3_add(state->camera.position, state->camera.front, center);
    glm_lookat(state->camera.position, center, state->camera.up, view);
    glm_mat4_copy(view, ubo.view);

    mat4 proj;
    float fovy = glm_rad(45.0f);
    float aspect = (float)state->surfaceInfo.capabilities.currentExtent.width / (float)state->surfaceInfo.capabilities.currentExtent.height;
    float nearZ = 0.1f;
    float farZ = 10.0f;
    glm_perspective(fovy, aspect, nearZ, farZ, proj);
    glm_mat4_copy(proj, ubo.proj);
    ubo.proj[1][1] *= -1;
    memcpy(state->objectState.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void processMouseInput(VkState* state){
    ImGuiIO* io = igGetIO_Nil();
    if (io->WantCaptureMouse){
        if (state->camera.mouseCaptured){
            glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            state->camera.mouseCaptured = false;
        }
        state->camera.firstMouse = true;
        return;
    }

    if (glfwGetMouseButton(state->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        if (!state->camera.mouseCaptured){
            state->camera.mouseCaptured = true;
            glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            state->camera.firstMouse = true;
        }
    } else if (state->camera.mouseCaptured){
        glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        state->camera.mouseCaptured = false;
        state->camera.firstMouse = true;
    }

    if (!state->camera.mouseCaptured)
        return;

    double xpos, ypos;
    glfwGetCursorPos(state->window, &xpos, &ypos);
    if (state->camera.firstMouse){
        state->camera.lastX = xpos;
        state->camera.lastY = ypos;
        state->camera.firstMouse = false;
    }

    float xoffset = (float)(state->camera.lastX - xpos) * state->camera.mouseSensitivity;
    float yoffset = (float)(state->camera.lastY - ypos) * state->camera.mouseSensitivity;
    state->camera.lastX = xpos;
    state->camera.lastY = ypos;

    state->camera.yaw += xoffset;
    state->camera.pitch += yoffset;
    if (state->camera.pitch > 89.0f) state->camera.pitch = 89.0f;
    if (state->camera.pitch < -89.0f) state->camera.pitch = -89.0f;
}

void updateCamera(VkState* state){
    vec3 front = {
        cosf(glm_rad(state->camera.pitch)) * cosf(glm_rad(state->camera.yaw)),
        cosf(glm_rad(state->camera.pitch)) * sinf(glm_rad(state->camera.yaw)),
        sinf(glm_rad(state->camera.pitch)),
    };
    glm_vec3_normalize_to(front, state->camera.front);

    vec3 worldUp = {0.0f, 0.0f, 1.0f};
    glm_vec3_crossn(state->camera.front, worldUp, state->camera.right);
    glm_vec3_crossn(state->camera.right, state->camera.front, state->camera.up);

    ImGuiIO* io = igGetIO_Nil();
    if (io->WantCaptureKeyboard)
        return;

    float velocity = state->camera.moveSpeed * state->deltaTime;
    vec3 planarFront;
    glm_vec3_scale(worldUp, glm_vec3_dot(state->camera.front, worldUp), planarFront);
    glm_vec3_sub(state->camera.front, planarFront, planarFront);
    if (glm_vec3_norm(planarFront) > 0.0f)
        glm_vec3_normalize(planarFront);
    else
        glm_vec3_copy(state->camera.front, planarFront);

    if (glfwGetKey(state->window, GLFW_KEY_W) == GLFW_PRESS)
        glm_vec3_muladds(planarFront, velocity, state->camera.position);
    if (glfwGetKey(state->window, GLFW_KEY_S) == GLFW_PRESS)
        glm_vec3_muladds(planarFront, -velocity, state->camera.position);

    if (glfwGetKey(state->window, GLFW_KEY_A) == GLFW_PRESS)
        glm_vec3_muladds(state->camera.right, -velocity, state->camera.position);
    if (glfwGetKey(state->window, GLFW_KEY_D) == GLFW_PRESS)
        glm_vec3_muladds(state->camera.right, velocity, state->camera.position);

    if (glfwGetKey(state->window, GLFW_KEY_SPACE) == GLFW_PRESS)
        state->camera.position[2] += velocity;
    if (glfwGetKey(state->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        state->camera.position[2] -= velocity;
}

void constructUI(VkState* state){
    static bool open = true;
    ImGuiIO* io = igGetIO_Nil();
    //igShowDemoWindow(&open);

    igSetNextWindowPos((ImVec2){10.0f, 10.0f}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    igSetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoMove;

    if (igBegin("MetricsOverlay", NULL, flags)) {
        float fps  = io->Framerate;
        float ms   = 1000.0f / (fps > 0.0f ? fps : 1.0f);
        igText("Frame: %.1f ms (%.1f FPS)", ms, fps);
        igText("Hold Mouse 1 to use Camera");
    }
    igEnd();
}

void drawFrame(VkState* state){
    vkWaitForFences(state->device, 1, &state->inFlightFences[state->currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = 
        vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX, state->imageAvailableSemaphores[state->currentFrame], VK_NULL_HANDLE, &imageIndex);
    if(result == VK_ERROR_OUT_OF_DATE_KHR){ recreateSwapChain(state); return;
    } else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)){ assert(0); }

    double now = glfwGetTime();
    state->deltaTime = (float)(now - state->lastFrameTime);
    state->lastFrameTime = now;
    if (state->deltaTime > 0.1f) state->deltaTime = 0.1f; // clamp after long pauses

    processMouseInput(state);
    updateCamera(state);

    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    igNewFrame();
    constructUI(state);
    igRender();
    ImDrawData* drawData = igGetDrawData();

    vkResetFences(state->device, 1, &state->inFlightFences[state->currentFrame]);
    vkResetCommandBuffer(state->commandBuffers[state->currentFrame], 0);
    updateUniformBuffer(state, state->currentFrame);
    recordCommandBuffer(state, state->commandBuffers[state->currentFrame], imageIndex, drawData);

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

void renderLoop(VkState* state){
    printf("[+] Entering Render Loop\n");
    state->lastFrameTime = glfwGetTime();
    while(!glfwWindowShouldClose(state->window)){
        glfwPollEvents();
        drawFrame(state);
    }
    vkDeviceWaitIdle(state->device);
}

int main(void){
    printf("[+] Running Vulkan Program\n");
    VkState state;
    memset(&state, 0, sizeof(state));

    initWindow(&state);
    initCamera(&state);
    createInstance(&state);
    if(checkValidationLayerSupport())
        setupDebugMessenger(&state);
    pickPhysicalDevice(&state);
    setupQueues(&state);
    createLogicalDevice(&state);
    retrieveQueues(&state);
    createSurface(&state);
    createSwapChain(&state);
    createImageViews(&state);
    createRenderPass(&state);
    initObjectState(&state);
    createShaders(&state);
    createDescriptorSetLayout(&state);
    createGraphicsPipeline(&state);
    createCommandPool(&state);
    createColorResource(&state);
    createDepthResource(&state);
    createFrameBuffers(&state);
    createTextureImage(&state);
    createTextureImageView(&state);
    createTextureSampler(&state);
    createVertexBuffer(&state);
    createIndexBuffer(&state);
    createUniformBuffers(&state);
    createDescriptorPool(&state);
    createDescriptorSets(&state);
    createCommandBuffers(&state);
    createSyncObjects(&state);
    initGui(&state);

    renderLoop(&state);
    cleanup(&state);
}

void cleanup(VkState* state){
    printf("[!] Cleaning up Instance\n");
    cleanupSwapchain(state);
    glfwDestroyWindow(state->window);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(NULL);

    vkDestroyBuffer(state->device, state->objectState.vertexBuffer, NULL);
    vkFreeMemory(state->device, state->objectState.vertexBufferMemory, NULL);
    vkDestroyBuffer(state->device, state->objectState.indexBuffer, NULL);
    vkFreeMemory(state->device, state->objectState.indexBufferMemory, NULL);

    vkDestroyDescriptorPool(state->device, state->objectState.descriptorPool, NULL);
    vkDestroyDescriptorPool(state->device, state->imguiState.descriptorPool, NULL);

    vkDestroyDescriptorSetLayout(state->device, state->objectState.descriptorSetLayout, NULL);
    vkDestroyPipeline(state->device, state->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(state->device, state->pipelineLayout, NULL);

    vkDestroyRenderPass(state->device, state->renderPass, NULL);

    for(uint32_t i = 0; i < state->MAX_FRAMES_IN_FLIGHT; i++){
        if(state->inFlightFences[i] != VK_NULL_HANDLE)
            vkDestroyFence(state->device, state->inFlightFences[i], NULL);
        if(state->imageAvailableSemaphores[i] != VK_NULL_HANDLE)
            vkDestroySemaphore(state->device, state->imageAvailableSemaphores[i], NULL);
        if(state->objectState.uniformBuffers[i] != VK_NULL_HANDLE)
            vkDestroyBuffer(state->device, state->objectState.uniformBuffers[i], NULL);
        if(state->objectState.uniformBuffersMemory[i] != VK_NULL_HANDLE)
            vkFreeMemory(state->device, state->objectState.uniformBuffersMemory[i], NULL);
    }

    vkDestroyShaderModule(state->device, state->vertexShader, NULL);
    vkDestroyShaderModule(state->device, state->fragmentShader, NULL);

    vkDestroySampler(state->device, state->objectState.textureSampler, NULL);
    vkDestroyImageView(state->device, state->objectState.textureImageView, NULL);
    vkDestroyImage(state->device, state->objectState.textureImage, NULL);
    vkFreeMemory(state->device, state->objectState.textureImageMemory, NULL);

    vkDestroyCommandPool(state->device, state->commandPool, NULL);
    vkDestroyCommandPool(state->device, state->transientPool, NULL);
    vkDestroyCommandPool(state->device, state->imguiState.commandPool, NULL);
    vkDestroyDevice(state->device, NULL);

    DestroyDebugUtilsMessengerEXT(state->instance, state->debugMessenger, NULL);

    vkDestroySurfaceKHR(state->instance, state->surface, NULL);
    vkDestroyInstance(state->instance, NULL);

    free(state->queueFamilyProperties); state->queueFamilyProperties = NULL;
    free(state->renderFinishedSemaphores); state->renderFinishedSemaphores = NULL;
    free(state->swapchainImages); state->swapchainImages = NULL;
    free(state->swapchainImageViews); state->swapchainImageViews = NULL;
    free(state->swapchainFramebuffers); state->swapchainFramebuffers = NULL;
    free(state->commandBuffers); state->commandBuffers = NULL;
    free(state->imageAvailableSemaphores); state->imageAvailableSemaphores = NULL;
    free(state->inFlightFences); state->inFlightFences = NULL;
    free(state->objectState.vertices); state->objectState.vertexBuffer = NULL;
    free(state->objectState.indices); state->objectState.indices = NULL;
    free(state->objectState.attributeDescriptions); state->objectState.attributeDescriptions = NULL;
    free(state->objectState.uniformBuffers); state->objectState.uniformBuffers = NULL;
    free(state->objectState.uniformBuffersMemory); state->objectState.uniformBuffersMemory = NULL;
    free(state->objectState.uniformBuffersMapped); state->objectState.uniformBuffersMapped = NULL;
    free(state->objectState.descriptorSets); state->objectState.descriptorSets = NULL;

    glfwTerminate();
}
