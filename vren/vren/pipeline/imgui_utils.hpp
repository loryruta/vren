#pragma once

#include <imgui.h>

#include "gpu_repr.hpp"

namespace vren::imgui_utils
{
    inline constexpr uint32_t to_im_color(uint32_t color)
    {
        // VREN color format: ARGB
        // ImGUI color format: ABGR
        return (color & 0xff00ff00) | ((color & 0xff) << 16) | ((color & 0xff0000) >> 16);
    }

    inline void add_world_space_text(
        ImDrawList* draw_list,
        vren::camera_data const& camera_data,
        glm::vec3 const& position,
        char const* text,
        ImColor text_color,
        ImColor bg_color
    )
    {
        auto viewport = ImGui::GetMainViewport();

        glm::vec4 p = camera_data.m_projection * camera_data.m_view * glm::vec4(position, 1.0f);

        if (p.z / p.w < 0.0f)
        {
            return;
        }

        auto text_size = ImGui::CalcTextSize(text);

        p.x = ((p.x / p.w) * 0.5f + 0.5f) * viewport->Size.x + viewport->Pos.x - text_size.x * 0.5f;
        p.y = (-(p.y / p.w) * 0.5f + 0.5f) * viewport->Size.y + viewport->Pos.y - text_size.y * 0.5f;

        draw_list->AddRectFilled(
            ImVec2(p.x - 10, p.y - 10), ImVec2(p.x + text_size.x + 10, p.y + text_size.y + 10), bg_color
        );
        draw_list->AddText(ImVec2(p.x, p.y), text_color, text);
    }
} // namespace vren::imgui_utils
