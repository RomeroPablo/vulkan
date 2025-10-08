#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

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
    printf("[+] Initializing Window]\n");
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
    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
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

    VkPhysicalDeviceFeatures enabledFeatures = {};
    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfos,
        .queueCreateInfoCount = queueCreateInfoCount,
        .pEnabledFeatures = &enabledFeatures,
        .enabledExtensionCount = 0,
        .ppEnabledLayerNames = &validationLayer,
        .enabledLayerCount = 1,
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

    renderLoop(&state);
    cleanup(&state);
}

void cleanup(struct VkState* state){
    printf("[!] Cleaning up Instance\n");
    DestroyDebugUtilsMessengerEXT(state->instance, state->debugMessenger, NULL);
    vkDestroySurfaceKHR(state->instance, state->surface, NULL);
    vkDestroyDevice(state->device, NULL);
    vkDestroyInstance(state->instance, NULL);

    glfwDestroyWindow(state->window);

    glfwTerminate();
}
