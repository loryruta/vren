#pragma once

namespace vren
{
    class SpecializedShaderModule
    {
    private:
        vren::shader_module const* m_shader_module;
        vren::shader_module_entry_point const* m_entry_point;
        std::vector<uint8_t> m_specialization_data;

    public:
        inline specialized_shader(vren::shader_module const& shader_module, std::string const& entry_point = "main") :
            m_shader_module(&shader_module),
            m_entry_point(&shader_module.get_entry_point(entry_point))
        {
            size_t specialization_data_size = 0;
            for (VkSpecializationMapEntry const& specialization_map_entry : shader_module.m_specialization_map_entries)
            {
                specialization_data_size += specialization_map_entry.size;
            }
            m_specialization_data.resize(specialization_data_size);
        }

        specialized_shader(vren::specialized_shader const& other) = delete;

        inline specialized_shader(vren::specialized_shader const&& other) :
            m_shader_module(other.m_shader_module),
            m_entry_point(other.m_entry_point),
            m_specialization_data(std::move(other.m_specialization_data))
        {
        }

        inline vren::shader_module const& get_shader_module() const { return *m_shader_module; }

        inline char const* get_entry_point() const { return m_entry_point->m_name.c_str(); }

        inline VkShaderStageFlags get_shader_stage() const { return m_entry_point->m_shader_stage; }

        inline void const* get_specialization_data() const { return m_specialization_data.data(); }

        inline size_t get_specialization_data_length() const { return m_specialization_data.size(); }

        inline bool has_specialization_data() const { return m_specialization_data.size() > 0; }

        inline void set_specialization_data(uint32_t constant_id, void const* data, size_t size)
        {
            size_t offset = 0;
            for (VkSpecializationMapEntry const& specialization_map_entry :
                 m_shader_module->m_specialization_map_entries)
            {
                if (specialization_map_entry.constantID == constant_id)
                {
                    assert(size == specialization_map_entry.size);

                    memcpy(&m_specialization_data.data()[offset], data, specialization_map_entry.size);
                    return;
                }

                offset += specialization_map_entry.size;
            }

            throw std::runtime_error("Failed to find specialization constant for the given ID");
        }

        inline void set_specialization_data(std::string const& constant_name, void const* data, size_t size)
        {
            set_specialization_data(m_shader_module->get_specialization_constant_id(constant_name), data, size);
        }
    };
}