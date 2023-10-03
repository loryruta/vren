#pragma once

#include <memory>
#include <vector>

#include "tiny_gltf.h"

#include "Context.hpp"
#include "base/operation_fork.hpp"
#include "material.hpp"
#include "texture_manager.hpp"
#include "vk_helpers/image.hpp"

#include "model.hpp"

namespace vren
{
    class tinygltf_parser
    {
    private:
        vren::context const* m_context;

    public:
        tinygltf_parser(vren::context const& context);

    private:
        void load_textures(std::filesystem::path const& model_folder, tinygltf::Model const& gltf_model);

        void load_materials(
            tinygltf::Model const& gltf_model, uint32_t texture_start_index, std::vector<vren::material>& materials
        );

        void linearize_node_hierarchy(
            tinygltf::Model const& gltf_model,
            tinygltf::Node const& gltf_node,
            glm::mat4 transform,
            std::vector<std::vector<vren::mesh_instance>>& mesh_instances
        );

        void load_meshes(tinygltf::Model const& gltf_model, uint32_t material_start_index, vren::model& parsed_model);

        void load_model(
            std::filesystem::path const& model_folder,
            tinygltf::Model const& gltf_model,
            vren::model& parsed_model,
            uint32_t material_start_index,
            std::vector<vren::material>& materials
        );

    public:
        void load_from_file(
            std::filesystem::path const& model_filename,
            vren::model& parsed_model,
            uint32_t material_start_index,
            std::vector<vren::material>& materials
        );
    };
} // namespace vren
