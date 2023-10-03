#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "wrappers/DescriptorSetLayoutInfo.hpp"
#include "wrappers/HandleDeleter.hpp"

namespace vren
{
    enum class ShaderType
    {
        Vertex,
        Fragment,
        Compute
    };

    class ShaderCompilationException : public std::exception
    {
    public:
        ShaderCompilationException() : std::exception() {}
    };

    class ShaderModule
    {
    private:
        struct EntryPoint
        {
            std::string m_name;
            VkShaderStageFlags m_shader_stage;
        };

        std::string m_glsl_code;

        std::string m_filename;
        std::shared_ptr<HandleDeleter<VkShaderModule>> m_handle;

        std::string m_name;

        std::vector<EntryPoint> m_entry_points;
        std::unordered_map<uint32_t, DescriptorSetLayoutInfo> m_descriptor_set_layouts;
        size_t m_push_constant_block_size;

        std::vector<std::string> m_specialization_constant_names;
        std::vector<VkSpecializationMapEntry> m_specialization_map_entries;

        explicit ShaderModule(std::string const& filename);

    public:
        ~ShaderModule();

        EntryPoint const& entry_point(std::string const& name) const;

        bool has_push_constant_block() const { return m_push_constant_block_size > 0; }

        VkSpecializationMapEntry const& specialization_constant(std::string const& name) const;
        uint32_t specialization_constant_id(std::string const& name) const { return specialization_constant(name).constantID; }

        void recompile();

        static void load_from_file(std::string const& filename);

    };
} // namespace vren
