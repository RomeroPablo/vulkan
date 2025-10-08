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
    VkDebugUtilsMessengerEXT debugMessenger;
};

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

void renderLoop(struct VkState* state){
    while(!glfwWindowShouldClose(state->window)){
        glfwPollEvents();
    }
}

void cleanup(struct VkState* state){
    DestroyDebugUtilsMessengerEXT(state->instance, state->debugMessenger, NULL);

    vkDestroyInstance(state->instance, NULL);

    glfwDestroyWindow(state->window);

    glfwTerminate();
}

int main(void){
    printf("[+] Running Vulkan Program\n");
    struct VkState state;
    memset(&state, 0, sizeof(state));

    initWindow(&state);
    createInstance(&state);
    setupDebugMessenger(&state);

    renderLoop(&state);
    cleanup(&state);
}
