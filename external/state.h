#pragma once
#include <cglm/cglm.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

struct vkGranularMem{
    size_t flagCount;
    const char* flagStrings[9];
};
struct vkHeap{
    size_t heapSize;

    size_t flagCount;
    const char* flagStrings[3];

    size_t granularMemCount;
    struct vkGranularMem** granularMemories;

};
struct PhysicalMemoryProperties{
        size_t heapCount;
        struct vkHeap* heaps;
};

struct ImguiState{
    VkDescriptorPool descriptorPool;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    struct PhysicalMemoryProperties physicalMemory;
    int graphicsPipelineIndex;
    int fragmentShaderIndex;
}typedef ImguiState;

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
    VkPipeline graphicsPipelineLine;
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

    ImguiState imguiState;

    struct Camera camera;
    struct ObjectState objectState;

    VkDebugUtilsMessengerEXT debugMessenger;
}typedef VkState;
