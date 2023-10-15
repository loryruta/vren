#pragma once

#include <functional>
#include <vector>

#include <volk.h>

#include "base/ResourceContainer.hpp"
#include "light.hpp"
#include "vk_api/command_graph/render_graph.hpp"
#include "vk_api/image/utils.hpp"
#include "vk_api/misc_utils.hpp"

namespace vren
{
    // Forward decl
    class context;

    // --------------------------------------------------------------------------------------------------------------------------------
    // Swapchain frame data
    // --------------------------------------------------------------------------------------------------------------------------------

    struct swapchain_frame_data
    {
        vren::vk_semaphore m_image_available_semaphore;
        vren::vk_semaphore m_render_finished_semaphore;

        vren::vk_fence m_frame_fence;

        /**
         * The resource container holds the resources that are in-use by the frame and ensures they live enough.
         */
        vren::ResourceContainer m_resource_container;

        swapchain_frame_data(vren::context const& context);
    };

    // --------------------------------------------------------------------------------------------------------------------------------
    // Swapchain
    // --------------------------------------------------------------------------------------------------------------------------------

    class swapchain
    {
    private:
        vren::context const* m_context;

    public:
        VkSwapchainKHR m_handle;

        uint32_t m_image_width, m_image_height;
        VkSurfaceFormatKHR m_surface_format;
        VkPresentModeKHR m_present_mode;

        std::vector<VkImage> m_images;
        std::vector<vren::vk_image_view> m_image_views;

        std::array<vren::swapchain_frame_data, VREN_MAX_FRAME_IN_FLIGHT_COUNT> m_frame_data;

        swapchain(
            vren::context const& context,
            VkSwapchainKHR handle,
            uint32_t image_width,
            uint32_t image_height,
            VkSurfaceFormatKHR surface_format,
            VkPresentModeKHR present_mode
        );
        swapchain(swapchain const& other) = delete;
        swapchain(swapchain&& other);
        ~swapchain();

    private:
        void swap(swapchain& other);

        std::vector<VkImage> get_swapchain_images();

    public:
        swapchain& operator=(swapchain const& other) = delete;
        swapchain& operator=(swapchain&& other);
    };

    // --------------------------------------------------------------------------------------------------------------------------------
    // Presenter
    // --------------------------------------------------------------------------------------------------------------------------------

    class presenter
    {
    public:
        static constexpr std::initializer_list<char const*> s_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    private:
        vren::context const* m_context;
        vren::vk_surface_khr const* m_surface;
        std::function<void(vren::swapchain const& swapchain)> m_swapchain_recreate_callback;

        std::unique_ptr<vren::swapchain> m_swapchain;

        uint32_t m_present_queue_family_idx = -1;
        uint32_t m_current_frame_idx = 0;

    public:
        presenter(
            vren::context const& context,
            vren::vk_surface_khr const& surface,
            std::function<void(vren::swapchain const& swapchain)> const& swapchain_recreate_callback
        );

    private:
        uint32_t pick_min_image_count(vren::vk_utils::surface_details const& surf_details);
        VkSurfaceFormatKHR pick_surface_format(vren::vk_utils::surface_details const& surf_details);
        VkPresentModeKHR pick_present_mode(vren::vk_utils::surface_details const& surf_details);

    public:
        inline vren::swapchain* get_swapchain() const { return m_swapchain.get(); }

        void recreate_swapchain(uint32_t width, uint32_t height);

        using render_func_t = std::function<void(
            uint32_t frame_idx,
            uint32_t swapchain_image_idx,
            vren::swapchain const& swapchain,
            VkCommandBuffer command_buffer,
            vren::ResourceContainer& resource_container
        )>;
        void present(render_func_t const& render_func);
    };

    // --------------------------------------------------------------------------------------------------------------------------------

    vren::render_graph_t blit_color_buffer_to_swapchain_image(
        vren::render_graph_allocator& allocator,
        vren::vk_utils::color_buffer_t const& color_buffer,
        uint32_t width,
        uint32_t height,
        VkImage swapchain_image,
        uint32_t swapchain_image_width,
        uint32_t swapchain_image_height
    );

    vren::render_graph_t
    transit_swapchain_image_to_present_layout(vren::render_graph_allocator& allocator, VkImage swapchain_image);
} // namespace vren
