//
// Created by Akhmad on 11/3/25.
//

#ifndef CODE2LOGIC_SHADER_RESOURCE_HPP
#define CODE2LOGIC_SHADER_RESOURCE_HPP

#include "core/resources/i_resource.hpp"
#include "core/file_system/i_file_system.hpp"
#include "core/resources/shader_types.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <unordered_map>

namespace c2l::core::resources
{
    /**
     * @brief Shader resource for loading and managing OpenGL shaders
     *
     * Supports vertex, fragment, geometry, tessellation, and compute shaders
     * with automatic uniform detection and management.
     */
    class ShaderResource final : public IResource
    {
    public:
        /**
         * @brief Construct a shader resource
         * @param path Path to the shader file or directory
         * @param file_system File system for loading
         */
        ShaderResource(const std::string& path,
                       filesystem::IFileSystem& file_system);

        ~ShaderResource() override;

        // IResource implementation
        /**
         * @copydoc IResource::get_path
         */
        [[nodiscard]] const std::string& get_path() const override { return m_path; }


        /**
         * @copydoc IResource::get_state
         */
        [[nodiscard]] ResourceState get_state() const override { return m_state; }

        /**
         * @copydoc IResource::is_loaded
         */
        [[nodiscard]] bool is_loaded() const override;

        /**
         * @note CPU-side memory only
         *
         * @copydoc IResource::get_memory_usage
         */
        [[nodiscard]] uint64_t get_memory_usage() const override;

        /**
         * @copydoc IResource::get_last_access_time
         */
        [[nodiscard]] std::chrono::steady_clock::time_point get_last_access_time() const override;

        /**
         * @copydoc IResource::load
         */
        bool load() override;

        /**
         * @copydoc IResource::unload
         */
        void unload() override;

        /**
         * @copydoc IResource::update_access_time
         */
        void update_access_time() override;

        // Shader specific interface
        /**
         * @brief Use this shader program
         *
         * @note Must be called with a valid OpenGL context
         *       on the current thread.
         */
        void use() const;

        /**
         * @brief Get OpenGL program ID
         * @return GLuint program ID, 0 if not loaded
         */
        [[nodiscard]] GLuint get_program_id() const { return m_program_id; }

        /**
         * @brief Check if shader program is valid
         * @return true if valid, false otherwise
         */
        [[nodiscard]] bool is_valid() const;

        // Uniform setting methods
        void set_uniform(const std::string& name, bool value);
        void set_uniform(const std::string& name, int value);
        void set_uniform(const std::string& name, float value);
        void set_uniform(const std::string& name, const glm::vec2& value);
        void set_uniform(const std::string& name, const glm::vec3& value);
        void set_uniform(const std::string& name, const glm::vec4& value);
        void set_uniform(const std::string& name, const glm::mat3& value);
        void set_uniform(const std::string& name, const glm::mat4& value);
        void set_uniform(const std::string& name, const std::vector<float>& values);
        void set_uniform(const std::string& name, const std::vector<glm::vec3>& values);
        void set_uniform(const std::string& name, const std::vector<glm::mat4>& values);

        /**
         * @brief Get uniform location
         * @param name Uniform name
         * @return Location ID, -1 if not found
         */
        [[nodiscard]] GLint get_uniform_location(const std::string& name);

        /**
         * @brief Get all uniform information
         * @return Map of uniform names to their types and locations
         */
        [[nodiscard]] const auto& get_uniforms() const { return m_uniforms; }

        /**
         * @brief Reload shader from disk (for hot-reloading)
         * @return true if successful, false otherwise
         */
        bool reload();

        /**
         *@brief Get shader compilation/linking log
         *@return Log string messages
         */
        [[nodiscard]] const std::string& get_log() const
        {
            return m_log;
        }

        /**
         * @brief Get Supported shader types for this program
         * @return Vector of shader types used
         */
        [[nodiscard]] const std::vector<ShaderType>& get_shader_types()
        {
            return m_shader_types;
        }

    private:
        struct UniformInfo
        {
            GLint location;
            UniformType type;
            std::string name;
        };

        bool compile_shader(const std::string& source,
                            ShaderType type,
                            GLuint& shader_id);
        bool link_program();
        void detect_uniforms();

        void cleanup_shaders();

        [[nodiscard]] const UniformInfo* find_uniform(const std::string& name) const;
        [[nodiscard]] bool validate_uniform(const std::string& name, UniformType expected) const;
        [[nodiscard]] std::string get_shader_extension(ShaderType type) const;
        [[nodiscard]] ShaderType get_shader_type_from_extension(const std::string& extension) const;

        bool load_single_file();
        bool load_separate_files();

        std::string m_path;
        ResourceState m_state       {ResourceState::Unloaded};
        filesystem::IFileSystem& m_file_system;
        std::chrono::steady_clock::time_point m_last_access_time;

        GLuint m_program_id         {0};
        std::vector<GLuint> m_shader_ids;
        std::unordered_map<std::string, UniformInfo> m_uniforms;
        std::vector<ShaderType> m_shader_types;
        std::string m_log;
        uint64_t m_memory_usage     {0};
        bool m_is_separate_files    {false};
    };
} // namespace c2l::core::resources




#endif //CODE2LOGIC_SHADER_RESOURCE_HPP