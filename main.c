#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

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
    struct SwapchainInfo{
        VkSurfaceCapabilitiesKHR capabilities;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR presentMode;
    }swapchainInfo;
    VkExtent2D extent;
    uint32_t imageCount;
    VkSwapchainKHR swapchain;
    VkImage* swapchainImages;
    VkImageView* swapchainImageViews;
    VkPipeline graphicsPipeline;
    VkShaderModule vertexShader;
    VkShaderModule fragmentShader;

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

void initWindow(struct VkState* state){
    printf("[+] Initializing Window\n");
    glfwInit();
    state->resolution.width  = 800;
    state->resolution.height = 600;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    state->window = glfwCreateWindow(state->resolution.width, 
            state->resolution.height, "Vulkan", NULL, NULL);
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

void createSwapChain(struct VkState* state){
    printf("[+] Creating Swapchain\n");
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &state->swapchainInfo.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(state->physicalDevice, state->surface, &formatCount, formats);
    for(int i = 0; i < formatCount; i++){
        if((formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)){
            printf("[!] Found desired format!\n");
            state->swapchainInfo.format = formats[i];
            break;
        }
    }
    printf("[+] Using Swapchain format %i with colorspace %i\n", state->swapchainInfo.format.format, state->swapchainInfo.format.colorSpace);

    uint32_t presentCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentCount, NULL);
    VkPresentModeKHR presentModes[presentCount];
    vkGetPhysicalDeviceSurfacePresentModesKHR(state->physicalDevice, state->surface, &presentCount, presentModes);
    for(int i = 0; i < presentCount; i++){
        if(presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR){
            state->swapchainInfo.presentMode = presentModes[i];
            break;
        }
    }
    printf("[+] Using present mode %i\n", state->swapchainInfo.presentMode);

    if(state->swapchainInfo.capabilities.currentExtent.height != UINT32_MAX){
        state->extent = state->swapchainInfo.capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(state->window, &width, &height);
        state->extent.width = (uint32_t)(width);
        state->extent.height= (uint32_t)(height);
        state->extent.width = clamp(state->extent.width, 
                state->swapchainInfo.capabilities.minImageExtent.width, state->swapchainInfo.capabilities.maxImageExtent.width);
        state->extent.height = clamp(state->extent.height, 
                state->swapchainInfo.capabilities.minImageExtent.height, state->swapchainInfo.capabilities.maxImageExtent.height);
    }
    state->imageCount = state->swapchainInfo.capabilities.minImageCount + 1;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .minImageCount = state->swapchainInfo.capabilities.minImageCount,
        .imageFormat = state->swapchainInfo.format.format,
        .imageColorSpace = state->swapchainInfo.format.colorSpace,
        .imageExtent = state->extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = state->queueFamilyIndices.graphicsFamily,
        .pQueueFamilyIndices = NULL,
        .preTransform = state->swapchainInfo.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = state->swapchainInfo.presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    assert(vkCreateSwapchainKHR(state->device, &createInfo, NULL, &state->swapchain) == VK_SUCCESS);

    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->imageCount, NULL);
    state->swapchainImages = calloc(state->imageCount, sizeof(VkImage) * state->imageCount);
    vkGetSwapchainImagesKHR(state->device, state->swapchain, &state->imageCount, state->swapchainImages);
}

void createImageViews(struct VkState* state){
    printf("[+] Creating Image Views\n");
    state->swapchainImageViews = calloc(state->imageCount, sizeof(VkImageView) * state->imageCount);
    for(int i = 0; i < state->imageCount; i++){
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = state->swapchainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = state->swapchainInfo.format.format,
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

}

void renderLoop(struct VkState* state){
    printf("[+] Entering Render Loop\n");
    while(!glfwWindowShouldClose(state->window)){
        glfwPollEvents();
    }
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
    createShaders(&state);
    createGraphicsPipeline(&state);

    renderLoop(&state);
    cleanup(&state);
}

void cleanup(struct VkState* state){
    printf("[!] Cleaning up Instance\n");
    DestroyDebugUtilsMessengerEXT(state->instance, state->debugMessenger, NULL);
    for(int i = 0; i < state->imageCount; i++){
        vkDestroyImageView(state->device, state->swapchainImageViews[i], NULL);
    }
    vkDestroyShaderModule(state->device, state->vertexShader, NULL);
    vkDestroyShaderModule(state->device, state->fragmentShader, NULL);
    vkDestroySwapchainKHR(state->device, state->swapchain, NULL);
    vkDestroySurfaceKHR(state->instance, state->surface, NULL);
    vkDestroyDevice(state->device, NULL);
    vkDestroyInstance(state->instance, NULL);

    glfwDestroyWindow(state->window);
    free(state->swapchainImages); state->swapchainImages = NULL;
    free(state->swapchainImageViews); state->swapchainImageViews = NULL;
    
    glfwTerminate();
}
