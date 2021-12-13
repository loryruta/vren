#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "vk_utils.hpp"
#include "gpu_allocator.hpp"
#include "light.hpp"

namespace vren
{
	// Forward decl
	class renderer;

	struct material
	{
		vren::renderer& m_renderer; // todo tmp

		uint32_t m_idx;

		vren::texture m_albedo_texture;

		float m_metallic;
		float m_roughness;

		explicit material(vren::renderer& renderer);
		~material();
	};

	class material_manager
	{
	private:
		std::vector<std::unique_ptr<material>> m_materials;

		vren::renderer& m_renderer;

		size_t m_material_struct_aligned_size;

		vren::gpu_buffer m_uniform_buffer;
		size_t m_uniform_buffer_size;

	public:
		explicit material_manager(vren::renderer& renderer);
		~material_manager();

		material* create_material();
		material* get_material(uint32_t idx);

		void upload_device_buffer();

		void update_material_descriptor_set(vren::material const* material, VkDescriptorSet descriptor_set);
	};
}
