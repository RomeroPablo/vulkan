#pragma once
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_VULKAN
#define CIMGUI_USE_GLFW
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
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

// this is only going to run once, so perf not too important (though it may hurt ttff)
static inline void updateMemoryProperties(ImguiState* state, const VkPhysicalDeviceMemoryProperties* props){
    state->physicalMemory.heapCount = props->memoryHeapCount;
    state->physicalMemory.heaps = (struct vkHeap*) calloc(state->physicalMemory.heapCount, sizeof(struct vkHeap));

    for (uint32_t i = 0; i < props->memoryHeapCount; ++i) {
        struct vkHeap* heap = &state->physicalMemory.heaps[i];

        heap->heapSize = props->memoryHeaps[i].size;
        heap->flagCount = extractHeapFlags(props->memoryHeaps[i].flags, heap->flagStrings, 3);

        heap->granularMemories = (struct vkGranularMem**)calloc(props->memoryTypeCount, sizeof(struct vkGranularMem*));
        heap->granularMemCount = 0;

    for (uint32_t j = 0; j < props->memoryTypeCount; ++j) {
        if (props->memoryTypes[j].heapIndex != i) continue;

        heap->granularMemories[heap->granularMemCount] = (struct vkGranularMem*)calloc(1, sizeof(struct vkGranularMem));
        heap->granularMemories[heap->granularMemCount]->flagCount = vkGetMemoryPropertyFlagStrings(props->memoryTypes[j].propertyFlags, heap->granularMemories[heap->granularMemCount]->flagStrings, 9);
        heap->granularMemCount++;
        }
    }
}

void static inline drawMemoryProperties(ImguiState* state){
    for (int i = 0; i < state->physicalMemory.heapCount; i++) {
        struct vkHeap* heap = &state->physicalMemory.heaps[i];
        igSeparator();
        igText("Heap %d | Size %.2f MB |", i, (float)heap->heapSize / (1024 * 1024));
        igSameLine(0.0f, -1.0f);
        for (int j = 0; j < heap->flagCount; j++) {
            igSameLine(0.0f, -1.0f);
            igText("[%s]", heap->flagStrings[j]);
        }

        char label[64];
        snprintf(label, sizeof(label),"Memory Types##%d", i);
        if (igTreeNode_Str(label)) {
            for (int j = 0; j < heap->granularMemCount; j++) {
                for (int k = 0; k < heap->granularMemories[j]->flagCount; k++) {
                    igText("[%s]", heap->granularMemories[j]->flagStrings[k]);
                    igSameLine(0.0f, -1.0f);
                }
                igNewLine();
            }
            igTreePop();
        }
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
        drawMemoryProperties(&state->imguiState);
    }
    igEnd();
}
