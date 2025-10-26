#pragma once
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_VULKAN
#define CIMGUI_USE_GLFW
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <string.h>
#include "cimgui.h"
#include "state.h"


static inline void setEngineStyle(ImGuiStyle* style){
    style->Colors[ImGuiCol_ChildBg]           		= (ImVec4){0.0f, 0.0f, 0.0f, 0.9f};
    style->Colors[ImGuiCol_PopupBg]           		= (ImVec4){0.05f, 0.05f, 0.05f, 0.9f};
    style->Colors[ImGuiCol_Border]            		= (ImVec4){0.2f, 0.2f, 0.2f, 1.0f};
    style->Colors[ImGuiCol_Separator]         		= (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_Text]              		= (ImVec4){0.9f, 0.9f, 0.9f, 1.0f};
    style->Colors[ImGuiCol_TextDisabled]      		= (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};
    style->Colors[ImGuiCol_Header]            		= (ImVec4){0.15f, 0.15f, 0.15f, 1.0f};
    style->Colors[ImGuiCol_HeaderHovered]     		= (ImVec4){0.25f, 0.25f, 0.25f, 1.0f};
    style->Colors[ImGuiCol_HeaderActive]      		= (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_TitleBg]           		= (ImVec4){0.1f, 0.1f, 0.1f, 1.0f};
    style->Colors[ImGuiCol_TitleBgActive]     		= (ImVec4){0.2f, 0.2f, 0.2f, 1.0f};
    style->Colors[ImGuiCol_TitleBgCollapsed]  		= (ImVec4){0.0f, 0.0f, 0.0f, 0.7f};
    style->Colors[ImGuiCol_Button]            		= (ImVec4){0.2f, 0.2f, 0.2f, 1.0f};
    style->Colors[ImGuiCol_ButtonHovered]     		= (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_ButtonActive]      		= (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_SliderGrab]        		= (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_SliderGrabActive]  		= (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};
    style->Colors[ImGuiCol_CheckMark]         		= (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};
    style->Colors[ImGuiCol_FrameBg]           		= (ImVec4){0.1f, 0.1f, 0.1f, 1.0f};
    style->Colors[ImGuiCol_FrameBgHovered]    		= (ImVec4){0.2f, 0.2f, 0.2f, 0.9f};
    style->Colors[ImGuiCol_FrameBgActive]     		= (ImVec4){0.3f, 0.3f, 0.3f, 0.9f};
    style->Colors[ImGuiCol_Tab]               		= (ImVec4){0.15f, 0.15f, 0.15f, 1.0f};
    style->Colors[ImGuiCol_TabHovered]        		= (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_ResizeGrip]        		= (ImVec4){0.2f, 0.2f, 0.2f, 0.2f};
    style->Colors[ImGuiCol_ResizeGripHovered] 		= (ImVec4){0.3f, 0.3f, 0.3f, 0.5f};
    style->Colors[ImGuiCol_ResizeGripActive]  		= (ImVec4){0.4f, 0.4f, 0.4f, 0.7f};
    style->Colors[ImGuiCol_ScrollbarBg]       		= (ImVec4){0.0f, 0.0f, 0.0f, 0.5f};
    style->Colors[ImGuiCol_ScrollbarGrab]     		= (ImVec4){0.2f, 0.2f, 0.2f, 0.7f};
    style->Colors[ImGuiCol_ScrollbarGrabHovered]    = (ImVec4){0.3f, 0.3f, 0.3f, 0.8f};
    style->Colors[ImGuiCol_ScrollbarGrabActive]     = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_PlotLines]         		= (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};
    style->Colors[ImGuiCol_PlotLinesHovered]  		= (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};
    style->Colors[ImGuiCol_PlotHistogram]     		= (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_PlotHistogramHovered]    = (ImVec4){0.6f, 0.6f, 0.6f, 1.0f};
    style->Colors[ImGuiCol_DragDropTarget]    		= (ImVec4){1.0f, 1.0f, 1.0f, 0.9f};
    style->Colors[ImGuiCol_NavWindowingHighlight]   = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_ModalWindowDimBg]  		= (ImVec4){0.0f, 0.0f, 0.0f, 0.8f};
    style->Colors[ImGuiCol_DockingEmptyBg]    		= (ImVec4){0.0f, 0.0f, 0.0f, 0.0f};
}

static inline void topLeft(ImGuiIO* io, bool render){
    igSetNextWindowPos((ImVec2){io->DisplaySize.x - 270.0f, 10.0f}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
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
        igText("Hold Mouse1 to use Camera");
        if(render)
        igText("Press r to hide the UI");
        else
        igText("Press r to show the UI");
    }
    igEnd();
}

static inline size_t vkGetMemoryPropertyFlagStrings(
    VkMemoryPropertyFlags flags,
    const char* outStrings[],
    size_t maxCount)
{
    size_t count = 0;
    if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && count < maxCount)
        outStrings[count++] = "DEVICE_LOCAL";
    if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && count < maxCount)
        outStrings[count++] = "HOST_VISIBLE";
    if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && count < maxCount)
        outStrings[count++] = "HOST_COHERENT";
    if ((flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) && count < maxCount)
        outStrings[count++] = "HOST_CACHED";
    if ((flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) && count < maxCount)
        outStrings[count++] = "LAZILY_ALLOCATED";
    if ((flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) && count < maxCount)
        outStrings[count++] = "PROTECTED";
    if ((flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) && count < maxCount)
        outStrings[count++] = "DEVICE_COHERENT_AMD";
    if ((flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD) && count < maxCount)
        outStrings[count++] = "DEVICE_UNCACHED_AMD";
    if ((flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV) && count < maxCount)
        outStrings[count++] = "RDMA_CAPABLE_NV";
    return count;
}

/*
 * given vkphysicaldevicememoryprops we should
 * for i < memoryheapcount
 * memoryheaps[i] <- extract size of the heap and flags of the heap
 *
 * for i < memorytypecount
 * memorytypes[i] <- extract heap index and flags of the memory
 *
 * really, we only have to do this once
 * we should store it in a hierarchy
 *
 * memHierarchy{
 *  heap_t heaps[] {
 *         memoryflags
 *         memoryflags
 *         memoryflags
 *         ...
 *  }
 * }
 */

static inline size_t extractHeapFlags(VkMemoryHeapFlags flags,
        const char* outStrings[],
        size_t maxCount){
    size_t count = 0;
    if ((flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) && count < maxCount)
        outStrings[count++] = "DEVICE_LOCAL";
    if ((flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT ) && count < maxCount)
        outStrings[count++] = "MULTI_INSTANCE";
    if ((flags & VK_MEMORY_HEAP_TILE_MEMORY_BIT_QCOM ) && count < maxCount)
        outStrings[count++] = "TILE_MEMORY_QCOM";
    if((flags == 0) && count < maxCount){
        outStrings[count++] = "NO_HEAP_FLAGS";
    }
    return count;
}

// this is only going to run once, so perf not too important (though it may hurt ttff)
void static inline updateMemoryProperties(ImguiState* state, VkPhysicalDeviceMemoryProperties* props){
    state->physicalMemory.heapCount = props->memoryHeapCount;
    state->physicalMemory.heaps = (struct vkHeap*)malloc(sizeof(struct vkHeap) * props->memoryHeapCount);
    for(int i = 0; i < props->memoryHeapCount; i++){
        state->physicalMemory.heaps[i].size = props->memoryHeaps[i].size;
        state->physicalMemory.heaps[i].flagCount = 
            extractHeapFlags(props->memoryHeaps[i].flags, state->physicalMemory.heaps[i].flagStrings, 3);
    }
}

// how should we draw it?
// for every memory heap
// print out the size, and its memory subsets
void static inline drawMemoryProperties(ImguiState* state){
    for(int i = 0; i < state->physicalMemory.heapCount; i++){
        igText("Heap %i:", i);
        igText("Size %.2f MB:", (float)state->physicalMemory.heaps[i].size / (1024 * 1024 ));
        for(int j = 0; j < state->physicalMemory.heaps[i].flagCount; j++){
            igText("Flags %s", state->physicalMemory.heaps[i].flagStrings[j]);
        };
    }
}

void static inline draw_memory_properties_ui(VkPhysicalDeviceMemoryProperties* props)
{
    igText("Memory Heaps: %u", props->memoryHeapCount);
    for (uint32_t i = 0; i < props->memoryHeapCount; i++) {
        igBulletText("Heap %u: size = %.2f MB, flags = 0x%08x",
                     i,
                     (float)props->memoryHeaps[i].size / (1024.0f * 1024.0f),
                     props->memoryHeaps[i].flags);
    }

    igSeparator();
    igText("Memory Types: %u", props->memoryTypeCount);

    for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
        const char* flagStrings[9];
        size_t flagCount = vkGetMemoryPropertyFlagStrings(
            props->memoryTypes[i].propertyFlags, flagStrings, 9);

        char buffer[256] = {0};
        for (size_t j = 0; j < flagCount; j++) {
            strcat(buffer, flagStrings[j]);
            if (j + 1 < flagCount)
                strcat(buffer, " | ");
        }

        igBulletText("Type %u: heap %u  [%s]",
                     i,
                     props->memoryTypes[i].heapIndex,
                     buffer[0] ? buffer : "None");
    }
}

static inline void staticInfo(VkState* state, ImGuiIO* io){
    igSetNextWindowPos((ImVec2){0.0f, 0.0f}, ImGuiCond_Always, (ImVec2){0.0f, 0.0f});
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoDecoration;
    uint32_t apiVersion = state->physicalDeviceProperties.apiVersion;
    const char* names[9];
    size_t count;
    if(igBegin("StaticUtils", NULL, flags)){
        igText("Physical Device Name: %s", state->physicalDeviceProperties.deviceName);
        igText("Device ID: %i API: %u.%u.%u", state->physicalDeviceProperties.deviceID, VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion));
        draw_memory_properties_ui(&state->physicalMemoryProperties);
        drawMemoryProperties(&state->imguiState);
    }
    igEnd();
}
