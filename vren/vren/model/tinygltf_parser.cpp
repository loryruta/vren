#include "tinygltf_parser.hpp"

#include <optional>
#include <stdexcept>

#include <stb_image.h>
#include <tiny_gltf.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "pipeline/profiler.hpp"
#include "toolbox.hpp"

glm::vec3 parse_gltf_vec3_to_glm_vec3(std::vector<double> v)
{
    assert(v.size() == 3);

    glm::vec3 r{};
    r.x = (float) v.at(0);
    r.y = (float) v.at(1);
    r.z = (float) v.at(2);
    return r;
}

glm::vec4 parse_gltf_vec4_to_glm_vec4(std::vector<double> v)
{
    assert(v.size() == 4);

    glm::vec4 r{};
    r.x = (float) v.at(0);
    r.y = (float) v.at(1);
    r.z = (float) v.at(2);
    r.w = (float) v.at(3);
    return r;
}

glm::mat4 parse_gltf_mat4_to_glm_mat4(std::vector<double> v)
{
    assert(v.size() == 16);

    glm::mat4 r{};
    r[0] = glm::vec4(v[0], v[1], v[2], v[3]);
    r[1] = glm::vec4(v[4], v[5], v[6], v[7]);
    r[2] = glm::vec4(v[8], v[9], v[10], v[11]);
    r[3] = glm::vec4(v[12], v[13], v[14], v[15]);
    return r;
}

glm::quat parse_gltf_quat_to_glm_quat(std::vector<double> v)
{
    return glm::quat(v[3], v[0], v[1], v[2]);
}

uint8_t const* get_accessor_element_at(tinygltf::Model const& model, tinygltf::Accessor const& accessor, size_t idx)
{
    tinygltf::BufferView const& buf_view = model.bufferViews.at(accessor.bufferView);
    tinygltf::Buffer const& buf = model.buffers.at(buf_view.buffer);

    size_t byte_off = buf_view.byteOffset + accessor.byteOffset;
    size_t stride = accessor.ByteStride(buf_view);
    size_t pos = byte_off + stride * idx;

    return buf.data.data() + pos;
}

VkFilter parse_gltf_filter(int gltf_filter)
{
    switch (gltf_filter)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return VK_FILTER_NEAREST;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return VK_FILTER_LINEAR;
    default:
        return VK_FILTER_LINEAR; // todo
    }
}

VkSamplerAddressMode parse_gltf_address_mode(int gltf_address_mode)
{
    switch (gltf_address_mode)
    {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    default:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

vren::tinygltf_parser::tinygltf_parser(vren::context const& context) :
    m_context(&context)
{
}

void vren::tinygltf_parser::load_textures(std::filesystem::path const& model_folder, tinygltf::Model const& gltf_model)
{
    for (auto const& gltf_texture : gltf_model.textures)
    {
        tinygltf::Image const& gltf_image = gltf_model.images.at(gltf_texture.source);

        int img_w, img_h, img_comp;
        stbi_uc* img_data;

        if (gltf_image.bufferView >= 0)
        { /* Bundle texture loading */
            auto const& gltf_buf_view = gltf_model.bufferViews.at(gltf_image.bufferView);
            auto const& gltf_buf = gltf_model.buffers.at(gltf_buf_view.buffer);

            size_t byte_off = gltf_buf_view.byteOffset;
            size_t byte_str = gltf_buf_view.byteStride;
            size_t byte_len = gltf_buf_view.byteLength;

            if (byte_str != 0)
            {
                throw std::runtime_error("Unsupported image data format (byte stride > 0)");
            }

            img_data = stbi_load_from_memory(
                gltf_buf.data.data() + byte_off, (int) byte_len, &img_w, &img_h, &img_comp, STBI_rgb_alpha
            );
            if (img_data == nullptr)
            {
                fprintf(
                    stderr, "Failed to load texture %s, reason: %s\n", gltf_texture.name.c_str(), stbi_failure_reason()
                );
                throw std::runtime_error("Unsupported image data");
            }
        }
        else
        { /* From file texture loading */
            std::filesystem::path img_file = model_folder / gltf_image.uri;

            img_data = stbi_load(img_file.string().c_str(), &img_w, &img_h, &img_comp, STBI_rgb_alpha);
            if (img_data == nullptr)
            {
                fprintf(stderr, "Failed to load texture %ls, reason: %s\n", img_file.c_str(), stbi_failure_reason());
                throw std::runtime_error("Unsupported image data");
            }
        }

        /* Sampler */
        VkFilter min_filter = VK_FILTER_LINEAR, mag_filter = VK_FILTER_LINEAR;
        VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                             address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                             address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        if (gltf_texture.sampler >= 0)
        {
            auto& gltf_sampler = gltf_model.samplers.at(gltf_texture.sampler);

            min_filter = parse_gltf_filter(gltf_sampler.minFilter);
            mag_filter = parse_gltf_filter(gltf_sampler.magFilter);
            address_mode_u = parse_gltf_address_mode(gltf_sampler.wrapR);
            address_mode_v = parse_gltf_address_mode(gltf_sampler.wrapS);
            address_mode_w = parse_gltf_address_mode(gltf_sampler.wrapT);
        }

        /* */

        m_context->m_toolbox->m_texture_manager.m_textures.push_back(vren::vk_utils::create_texture(
            *m_context,
            img_w,
            img_h,
            img_data,
            VK_FORMAT_R8G8B8A8_UNORM,
            mag_filter,
            min_filter,
            VK_SAMPLER_MIPMAP_MODE_NEAREST,
            address_mode_u,
            address_mode_v,
            address_mode_w
        ));

        stbi_image_free(img_data);
    }
}

void vren::tinygltf_parser::load_materials(
    tinygltf::Model const& gltf_model, uint32_t texture_start_index, std::vector<vren::material>& materials
)
{
    for (tinygltf::Material const& gltf_material : gltf_model.materials)
    {
        tinygltf::PbrMetallicRoughness const& gltf_pbr = gltf_material.pbrMetallicRoughness;

        vren::material material{
            .m_base_color_texture_idx = gltf_pbr.baseColorTexture.index >= 0
                                            ? (uint32_t) (texture_start_index + gltf_pbr.baseColorTexture.index)
                                            : 0,
            .m_metallic_roughness_texture_idx =
                gltf_pbr.metallicRoughnessTexture.index >= 0
                    ? (uint32_t) (texture_start_index + gltf_pbr.metallicRoughnessTexture.index)
                    : 0,
            .m_metallic_factor = (float) gltf_pbr.metallicFactor,
            .m_roughness_factor = (float) gltf_pbr.roughnessFactor,
            .m_base_color_factor = parse_gltf_vec4_to_glm_vec4(gltf_pbr.baseColorFactor),
        };
        materials.push_back(material);
    }
}

void vren::tinygltf_parser::linearize_node_hierarchy(
    tinygltf::Model const& gltf_model,
    tinygltf::Node const& gltf_node,
    glm::mat4 transform,
    std::vector<std::vector<vren::mesh_instance>>& mesh_instances
)
{
    if (!gltf_node.matrix.empty())
    {
        transform *= parse_gltf_mat4_to_glm_mat4(gltf_node.matrix);
    }

    if (!gltf_node.translation.empty())
    {
        transform = glm::translate(transform, parse_gltf_vec3_to_glm_vec3(gltf_node.translation));
    }

    if (!gltf_node.rotation.empty())
    {
        transform *= glm::mat4_cast(parse_gltf_quat_to_glm_quat(gltf_node.rotation));
    }

    if (!gltf_node.scale.empty())
    {
        transform = glm::scale(transform, parse_gltf_vec3_to_glm_vec3(gltf_node.scale));
    }

    // Registers the node for the mesh
    if (gltf_node.mesh >= 0)
    {
        mesh_instances[gltf_node.mesh].push_back({.m_transform = transform});
    }

    // Traverses children
    if (gltf_node.children.size() > 0)
    {
        for (int child_idx : gltf_node.children)
        {
            linearize_node_hierarchy(gltf_model, gltf_model.nodes.at(child_idx), transform, mesh_instances);
        }
    }
}

void vren::tinygltf_parser::load_meshes(
    tinygltf::Model const& gltf_model, uint32_t material_start_index, vren::model& parsed_model
)
{
    // Mesh instances
    std::vector<std::vector<vren::mesh_instance>> instances_by_mesh(gltf_model.meshes.size());
    for (auto const& gltf_scene : gltf_model.scenes)
    {
        for (int node : gltf_scene.nodes)
        {
            linearize_node_hierarchy(
                gltf_model, gltf_model.nodes.at(node), glm::identity<glm::mat4>(), instances_by_mesh
            );
        }
    }

    // Meshes
    for (uint32_t mesh_idx = 0; mesh_idx < gltf_model.meshes.size(); mesh_idx++)
    {
        tinygltf::Mesh const& gltf_mesh = gltf_model.meshes.at(mesh_idx);
        for (auto const& gltf_primitive : gltf_mesh.primitives)
        {
            vren::model::mesh mesh{};

            // assert(gltf_primitive.mode == TINYGLTF_MODE_TRIANGLES);
            if (gltf_primitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                continue;
            }

            auto const& attribs = gltf_primitive.attributes;

            assert(attribs.contains("POSITION"));
            assert(attribs.contains("NORMAL"));

            auto const& position_accessor = gltf_model.accessors.at(attribs.at("POSITION"));
            auto const& normal_accessor = gltf_model.accessors.at(attribs.at("NORMAL"));
            auto const& texcoord_accessor = attribs.contains("TEXCOORD_0")
                                                ? std::make_optional(gltf_model.accessors.at(attribs.at("TEXCOORD_0")))
                                                : std::nullopt;
            // TANGENT

            assert(
                position_accessor.type == TINYGLTF_TYPE_VEC3 &&
                position_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT
            );
            assert(
                normal_accessor.type == TINYGLTF_TYPE_VEC3 &&
                normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT
            );
            assert(
                !texcoord_accessor.has_value() || (texcoord_accessor->type == TINYGLTF_TYPE_VEC2 &&
                                                   texcoord_accessor->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            );

            /* Vertices */
            size_t vtx_count = position_accessor.count;
            assert(vtx_count == normal_accessor.count);
            assert(!texcoord_accessor.has_value() || vtx_count == texcoord_accessor->count);

            mesh.m_vertex_count = vtx_count;
            mesh.m_vertex_offset = parsed_model.m_vertices.size();

            for (size_t vtx_idx = 0; vtx_idx < vtx_count; vtx_idx++)
            {
                vren::vertex vertex{};
                vertex.m_position =
                    *reinterpret_cast<glm::vec3 const*>(get_accessor_element_at(gltf_model, position_accessor, vtx_idx)
                    );
                vertex.m_normal =
                    *reinterpret_cast<glm::vec3 const*>(get_accessor_element_at(gltf_model, normal_accessor, vtx_idx));
                if (texcoord_accessor.has_value())
                {
                    vertex.m_texcoords = *reinterpret_cast<glm::vec2 const*>(
                        get_accessor_element_at(gltf_model, texcoord_accessor.value(), vtx_idx)
                    );
                }
                parsed_model.m_vertices.push_back(vertex);
            }

            /* Indices */
            auto const& indices_accessor = gltf_model.accessors.at(gltf_primitive.indices);

            mesh.m_index_count = indices_accessor.count;
            mesh.m_index_offset = parsed_model.m_indices.size();

            for (size_t i = 0; i < indices_accessor.count; i++)
            {
                uint32_t local_index;
                switch (indices_accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_BYTE:
                    local_index =
                        *reinterpret_cast<uint8_t const*>(get_accessor_element_at(gltf_model, indices_accessor, i));
                    break;
                case TINYGLTF_COMPONENT_TYPE_SHORT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    local_index =
                        *reinterpret_cast<uint16_t const*>(get_accessor_element_at(gltf_model, indices_accessor, i));
                    break;
                case TINYGLTF_COMPONENT_TYPE_INT:
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    local_index =
                        *reinterpret_cast<uint32_t const*>(get_accessor_element_at(gltf_model, indices_accessor, i));
                    break;
                default:
                    throw std::invalid_argument("Unsupported indices component type");
                }
                parsed_model.m_indices.push_back(local_index);
            }

            /* Material */
            mesh.m_material_idx = material_start_index + gltf_primitive.material;

            /* Instances */
            auto instances = instances_by_mesh.at(mesh_idx);

            mesh.m_instance_offset = parsed_model.m_instances.size();
            mesh.m_instance_count = instances.size();

            parsed_model.m_instances.insert(parsed_model.m_instances.end(), instances.begin(), instances.end());

            /* */
            parsed_model.m_meshes.push_back(mesh);
        }
    }
}

void vren::tinygltf_parser::load_model(
    std::filesystem::path const& model_folder,
    tinygltf::Model const& gltf_model,
    vren::model& parsed_model,
    uint32_t material_start_index,
    std::vector<vren::material>& materials
)
{
    uint32_t texture_start_index = (uint32_t) m_context->m_toolbox->m_texture_manager.m_textures.size();

    // Textures
    load_textures(model_folder, gltf_model);
    m_context->m_toolbox->m_texture_manager.rewrite_descriptor_set();

    // Materials
    load_materials(gltf_model, texture_start_index, materials);

    // Meshes
    load_meshes(gltf_model, material_start_index, parsed_model);
}

void vren::tinygltf_parser::load_from_file(
    std::filesystem::path const& model_filename,
    vren::model& parsed_model,
    uint32_t material_start_index,
    std::vector<vren::material>& materials
)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model gltf_model;
    std::string warning, error;

    parsed_model.m_name = model_filename.string();

    printf("Loading model file: %ls\n", model_filename.c_str());

    bool ret;
    if (model_filename.extension() == ".gltf")
        ret = loader.LoadASCIIFromFile(&gltf_model, &error, &warning, model_filename.string());
    else if (model_filename.extension() == ".glb")
        ret = loader.LoadBinaryFromFile(&gltf_model, &error, &warning, model_filename.string());
    else
    {
        throw std::runtime_error("Unsupported file extension");
    }

    if (!warning.empty())
    {
        fprintf(stderr, "%s loading warning: %s\n", model_filename.string().c_str(), warning.c_str());
        throw std::runtime_error("Loading failed");
    }

    if (!error.empty())
    {
        fprintf(stderr, "%s loading error: %s\n", model_filename.string().c_str(), error.c_str());
        throw std::runtime_error("Loading failed");
    }

    if (!ret)
    {
        throw std::runtime_error("Scene loading failed");
    }

    load_model(model_filename.parent_path(), gltf_model, parsed_model, material_start_index, materials);

    VREN_INFO("[tinygltf_parser] Model loaded: {}\n", parsed_model.m_name);
}
