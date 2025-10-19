// main.c
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_VULKAN
#define CIMGUI_USE_GLFW
#include "external/cimgui.h"
#include "external/cimgui_impl.h"
#include "external/style.h"

struct Vertex {
    vec2 pos;
    vec3 color;
};

struct UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

struct ObjectState {
    struct Vertex* vertices;
    uint32_t vertexCount;
    uint16_t* indices;
    uint16_t indexCount;
    VkVertexInputBindingDescription bindingDescription;
    VkVertexInputAttributeDescription* attributeDescriptions;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkBuffer* uniformBuffers;
    VkDeviceMemory* uniformBuffersMemory;
    void** uniformBuffersMapped;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet* descriptorSets;
};

struct ImguiState{
    VkDescriptorPool descriptorPool;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

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

    struct ObjectState objectState;
    struct ImguiState imguiState;

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

void setupDebugMessenger(struct VkState* state){
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

void pickPhysicalDevice(struct VkState* state){
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
    free(devices);
}

void setupQueues(struct VkState* state){
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

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = queueCreateInfoCount,
        .ppEnabledExtensionNames = enabledExtensions,
        .enabledExtensionCount = enabledExtensionCount,
        .pEnabledFeatures = &enabledFeatures,
        .ppEnabledLayerNames = NULL,
        .enabledLayerCount = 0
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
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
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

void initObjectState(struct VkState* state){
    state->objectState.vertexCount = 4;
    state->objectState.vertices = malloc(sizeof(struct Vertex) * state->objectState.vertexCount);

    struct Vertex vertTemp[] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
    };
    memcpy(state->objectState.vertices, vertTemp, sizeof(vertTemp));

    state->objectState.indexCount = 6;
    state->objectState.indices = malloc(sizeof(state->objectState.indices) * state->objectState.indexCount);

    uint16_t indices[] = {
        0, 1, 2, 2, 3, 0
    };
    memcpy(state->objectState.indices, indices, sizeof(indices));

    state->objectState.bindingDescription.binding = 0;
    state->objectState.bindingDescription.stride = sizeof(struct Vertex);
    state->objectState.bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    state->objectState.attributeDescriptions = malloc(2*sizeof(VkVertexInputAttributeDescription));
    state->objectState.attributeDescriptions[0].binding = 0;
    state->objectState.attributeDescriptions[0].location = 0;
    state->objectState.attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    state->objectState.attributeDescriptions[0].offset = offsetof(struct Vertex, pos);

    state->objectState.attributeDescriptions[1].binding = 0;
    state->objectState.attributeDescriptions[1].location = 1;
    state->objectState.attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    state->objectState.attributeDescriptions[1].offset = offsetof(struct Vertex, color);

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

void createDescriptorSetLayout(struct VkState* state){
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .binding = 0,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = NULL,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
    };

    assert(vkCreateDescriptorSetLayout(state->device, &layoutInfo, NULL, &state->objectState.descriptorSetLayout) == VK_SUCCESS);
}

void createGraphicsPipeline(struct VkState* state){
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

        .vertexAttributeDescriptionCount = 2,
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
        .pDepthStencilState = NULL,
        .pColorBlendState = &colorBlending,
        .layout = state->pipelineLayout,
        .renderPass = state->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    assert(vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &state->graphicsPipeline) == VK_SUCCESS);
}

void createFrameBuffers(struct VkState* state){
    printf("[+] Creating Framebuffers\n");
    // consider: https://erfan-ahmadi.github.io/blog/Nabla/fif for the future
    state->MAX_FRAMES_IN_FLIGHT = (2 > state->imageCount ) ? 2 : state->imageCount;
    printf("[+] Max Frames in Flight %i\n", state->imageCount);
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

    VkCommandPoolCreateInfo transientInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags =  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = state->queueFamilyIndices.graphicsFamily
    };
    assert(vkCreateCommandPool(state->device, &transientInfo, NULL, &state->transientPool) == VK_SUCCESS);
}

void createBuffer(struct VkState* state, 
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
    uint32_t index = -1;
    uint32_t typeFilter = memRequirements.memoryTypeBits;

    for(uint32_t i = 0; i < state->physicalMemoryProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i))
        && (state->physicalMemoryProperties.memoryTypes[i].propertyFlags) == properties){
            index = i;
        }
    }
    assert(index != -1);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = index
    };

    assert(vkAllocateMemory(state->device, &allocInfo, NULL, bufferMemory) == VK_SUCCESS);
    vkBindBufferMemory(state->device, *buffer, *bufferMemory, 0);
}

void copyBuffer(struct VkState* state, VkBuffer src, VkBuffer dst, VkDeviceSize size){
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
    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(state->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->graphicsQueue);
    vkFreeCommandBuffers(state->device, state->transientPool, 1, &commandBuffer);
}

void createVertexBuffer(struct VkState* state){
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

void createIndexBuffer(struct VkState* state){
    printf("[+] Creating Index Buffer\n");
    VkDeviceSize bufferSize = sizeof(state->objectState.indices) * state->objectState.indexCount;
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

void createUniformBuffers(struct VkState* state){
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

void createDescriptorPool(struct VkState* state){
    VkDescriptorPoolSize descriptorPoolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = state->MAX_FRAMES_IN_FLIGHT
    };
    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &descriptorPoolSize,
        .maxSets = state->MAX_FRAMES_IN_FLIGHT,
        .flags = 0,
    };

    assert(vkCreateDescriptorPool(state->device, &descriptorPoolCreateInfo, NULL, &state->objectState.descriptorPool) == VK_SUCCESS);
}

void createDescriptorSets(struct VkState* state){
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
        VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = state->objectState.descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &bufferInfo,
            .pImageInfo = NULL,
            .pTexelBufferView = NULL,
        };
        vkUpdateDescriptorSets(state->device, 1, &descriptorWrite, 0, NULL);
    }
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

void recordCommandBuffer(struct VkState* state, VkCommandBuffer commandBuffer, uint32_t imageIndex, ImDrawData* texture){
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

    VkBuffer vertexBuffers[] = {state->objectState.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &state->objectState.vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, state->objectState.indexBuffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipelineLayout, 
            0, 1, &state->objectState.descriptorSets[state->currentFrame], 0, NULL);

    vkCmdDrawIndexed(commandBuffer, state->objectState.indexCount, 1, 0, 0, 0);

    ImGui_ImplVulkan_RenderDrawData(texture, commandBuffer, VK_NULL_HANDLE);

    vkCmdEndRenderPass(commandBuffer);
    assert(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS);
}

void createSyncObjects(struct VkState* state){
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
    ImGui_ImplVulkan_SetMinImageCount(state->MAX_FRAMES_IN_FLIGHT);
    createImageViews(state);
    createFrameBuffers(state);
}

void updateUniformBuffer(struct VkState* state, uint32_t currentImage){
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
    float angle = glm_rad(90.0f) * deltaTime;
    float* axis = (vec3){0.0f, 0.0f, 1.0f};
    glm_rotate(model, angle, axis);
    glm_mat4_copy(model, ubo.model);

    mat4 view;
    float* eye = (vec3){2.0f, 2.0f, 2.0f};
    float* center = (vec3){0.0f, 0.0f, 0.0f};
    float* up = (vec3){0.0f, 0.0f, 1.0f};
    glm_lookat(eye, center, up, view);
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

void drawFrame(struct VkState* state){
    vkWaitForFences(state->device, 1, &state->inFlightFences[state->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    bool yk = true;
    VkResult result = 
        vkAcquireNextImageKHR(state->device, state->swapchain, UINT64_MAX, state->imageAvailableSemaphores[state->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR){ recreateSwapChain(state); return;
    } else if ((result != VK_SUCCESS) && (result != VK_SUBOPTIMAL_KHR)){ assert(0); }

    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    igNewFrame();
    igShowDemoWindow(&yk);
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

void renderLoop(struct VkState* state){
    printf("[+] Entering Render Loop\n");
    while(!glfwWindowShouldClose(state->window)){
        glfwPollEvents();
        drawFrame(state);
    }
    vkDeviceWaitIdle(state->device);
}

void initImGuiDescriptorPool(struct VkState* state){
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
void initGui(struct VkState* state){
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
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .RenderPass = state->renderPass,
        .Subpass = 0,
    };
    ImGui_ImplVulkan_Init(&initInfo);
    ImGuiStyle* style = igGetStyle();
    setEngineStyle(style);

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
    ImGui_ImplVulkan_NewFrame();
    vkEndCommandBuffer(state->imguiState.commandBuffer);
    vkQueueSubmit(state->graphicsQueue, 1, &si, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->graphicsQueue);
    vkFreeCommandBuffers(state->device, state->transientPool, 1, &state->imguiState.commandBuffer);
}

int main(void){
    printf("[+] Running Vulkan Program\n");
    struct VkState state;
    memset(&state, 0, sizeof(state));

    initWindow(&state);
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
    createFrameBuffers(&state);
    createCommandPool(&state);
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

void cleanup(struct VkState* state){
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

    vkDestroyCommandPool(state->device, state->commandPool, NULL);
    vkDestroyCommandPool(state->device, state->transientPool, NULL);
    vkDestroyCommandPool(state->device, state->imguiState.commandPool, NULL);
    vkDestroyDevice(state->device, NULL);

    DestroyDebugUtilsMessengerEXT(state->instance, state->debugMessenger, NULL);

    vkDestroySurfaceKHR(state->instance, state->surface, NULL);
    vkDestroyInstance(state->instance, NULL);

    free(state->queueFamilyProperties); state->queueFamilyProperties = NULL;
    free(state->swapchainImages); state->swapchainImages = NULL;
    free(state->swapchainImageViews); state->swapchainImageViews = NULL;
    free(state->swapchainFramebuffers); state->swapchainFramebuffers = NULL;
    free(state->commandBuffers); state->commandBuffers = NULL;
    free(state->imageAvailableSemaphores); state->imageAvailableSemaphores = NULL;
    free(state->inFlightFences); state->inFlightFences = NULL;
    free(state->objectState.vertices); state->objectState.vertexBuffer = NULL;
    free(state->objectState.attributeDescriptions); state->objectState.attributeDescriptions = NULL;
    free(state->objectState.uniformBuffers); state->objectState.uniformBuffers = NULL;
    free(state->objectState.uniformBuffersMemory); state->objectState.uniformBuffersMemory = NULL;
    free(state->objectState.uniformBuffersMapped); state->objectState.uniformBuffersMapped = NULL;
    free(state->objectState.descriptorSets); state->objectState.descriptorSets = NULL;

    glfwTerminate();
}
