#ifndef ENGINE_STYLE
#define ENGINE_STYLE
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#define CIMGUI_USE_VULKAN
#define CIMGUI_USE_GLFW
#include "cimgui.h"

static inline void setEngineStyle(ImGuiStyle* style){
    style->Colors[ImGuiCol_ChildBg]            = (ImVec4){0.0f, 0.0f, 0.0f, 0.9f};
    style->Colors[ImGuiCol_PopupBg]            = (ImVec4){0.05f, 0.05f, 0.05f, 0.9f};
    style->Colors[ImGuiCol_Border]             = (ImVec4){0.2f, 0.2f, 0.2f, 1.0f};
    style->Colors[ImGuiCol_Separator]          = (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_Text]               = (ImVec4){0.9f, 0.9f, 0.9f, 1.0f};
    style->Colors[ImGuiCol_TextDisabled]       = (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};
    style->Colors[ImGuiCol_Header]             = (ImVec4){0.15f, 0.15f, 0.15f, 1.0f};
    style->Colors[ImGuiCol_HeaderHovered]      = (ImVec4){0.25f, 0.25f, 0.25f, 1.0f};
    style->Colors[ImGuiCol_HeaderActive]       = (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_TitleBg]            = (ImVec4){0.1f, 0.1f, 0.1f, 1.0f};
    style->Colors[ImGuiCol_TitleBgActive]      = (ImVec4){0.2f, 0.2f, 0.2f, 1.0f};
    style->Colors[ImGuiCol_TitleBgCollapsed]   = (ImVec4){0.0f, 0.0f, 0.0f, 0.7f};
    style->Colors[ImGuiCol_Button]             = (ImVec4){0.2f, 0.2f, 0.2f, 1.0f};
    style->Colors[ImGuiCol_ButtonHovered]      = (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_ButtonActive]       = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_SliderGrab]         = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_SliderGrabActive]   = (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};
    style->Colors[ImGuiCol_CheckMark]          = (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};
    style->Colors[ImGuiCol_FrameBg]            = (ImVec4){0.1f, 0.1f, 0.1f, 1.0f};
    style->Colors[ImGuiCol_FrameBgHovered]     = (ImVec4){0.2f, 0.2f, 0.2f, 0.9f};
    style->Colors[ImGuiCol_FrameBgActive]      = (ImVec4){0.3f, 0.3f, 0.3f, 0.9f};
    style->Colors[ImGuiCol_Tab]                = (ImVec4){0.15f, 0.15f, 0.15f, 1.0f};
    style->Colors[ImGuiCol_TabHovered]         = (ImVec4){0.3f, 0.3f, 0.3f, 1.0f};
    style->Colors[ImGuiCol_ResizeGrip]         = (ImVec4){0.2f, 0.2f, 0.2f, 0.2f};
    style->Colors[ImGuiCol_ResizeGripHovered]  = (ImVec4){0.3f, 0.3f, 0.3f, 0.5f};
    style->Colors[ImGuiCol_ResizeGripActive]   = (ImVec4){0.4f, 0.4f, 0.4f, 0.7f};
    style->Colors[ImGuiCol_ScrollbarBg]        = (ImVec4){0.0f, 0.0f, 0.0f, 0.5f};
    style->Colors[ImGuiCol_ScrollbarGrab]      = (ImVec4){0.2f, 0.2f, 0.2f, 0.7f};
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = (ImVec4){0.3f, 0.3f, 0.3f, 0.8f};
    style->Colors[ImGuiCol_ScrollbarGrabActive]  = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_PlotLines]          = (ImVec4){0.5f, 0.5f, 0.5f, 1.0f};
    style->Colors[ImGuiCol_PlotLinesHovered]   = (ImVec4){1.0f, 1.0f, 1.0f, 1.0f};
    style->Colors[ImGuiCol_PlotHistogram]      = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_PlotHistogramHovered] = (ImVec4){0.6f, 0.6f, 0.6f, 1.0f};
    style->Colors[ImGuiCol_DragDropTarget]     = (ImVec4){1.0f, 1.0f, 1.0f, 0.9f};
    style->Colors[ImGuiCol_NavWindowingHighlight] = (ImVec4){0.4f, 0.4f, 0.4f, 1.0f};
    style->Colors[ImGuiCol_ModalWindowDimBg]   = (ImVec4){0.0f, 0.0f, 0.0f, 0.8f};
    style->Colors[ImGuiCol_DockingEmptyBg]     = (ImVec4){0.0f, 0.0f, 0.0f, 0.0f};
}

#endif
