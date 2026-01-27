//
// Created by Akhmad on 11/3/25.
//

#include "shader_resource.hpp"
#include "core/utils/logger/logger.hpp"

#include "glm/gtc/type_ptr.hpp"

namespace c2l::core::resources
{
    ShaderResource::ShaderResource(
        const std::filesystem::path& path,
        filesystem::IFileSystem& file_system)
        : IResource(path)
        ,  m_file_system{file_system}
        ,  m_last_access_time{std::chrono::steady_clock::now()}
    {
        const auto stats = m_file_system.get_file_stats(path);
        m_is_separate_files = stats.is_directory;
    }

    ShaderResource::~ShaderResource()
    {
        unload();
    }

    bool ShaderResource::load()
    {
        if (m_state == ResourceState::Loading)
        {
            LOG_WARNING("Shader resource already loading: {}", m_path.string());
            return false;
        }

        if (m_state == ResourceState::Loaded && m_program_id != 0)
        {
            update_access_time();
            return true;
        }

        m_state = ResourceState::Loading;
        m_log.clear();

        bool success = false;

        if (m_is_separate_files)
        {
            success = load_separate_files();
        }
        else
        {
            success = load_single_file();
        }

        if (success)
        {
            m_state = ResourceState::Loaded;
            detect_uniforms();
            update_access_time();
        }
        else
        {
            m_state = ResourceState::Error;
            unload();
            LOG_ERROR("ShaderResource failed to load: {}\nLog: {}", m_path.string(), m_log);
        }

        return success;
    }

    void ShaderResource::unload()
    {
        if (m_program_id != 0)
        {
            cleanup_shaders();
            glDeleteProgram(m_program_id);
            m_program_id = 0;
            LOG_DEBUG("ShaderResource OpenGL program deleted");
        }

        m_uniforms.clear();
        m_shader_types.clear();
        m_memory_usage = 0;
        m_state = ResourceState::Unloaded;
    }

    bool ShaderResource::is_loaded() const
    {
        return m_state == ResourceState::Loaded && m_program_id != 0;
    }

    uint64_t ShaderResource::get_memory_usage() const
    {
        return m_memory_usage;
    }

    void ShaderResource::update_access_time()
    {
        m_last_access_time = std::chrono::steady_clock::now();
    }

    void ShaderResource::use() const
    {
        if (m_state == ResourceState::Loaded && m_program_id != 0)
        {
            glUseProgram(m_program_id);
        }
    }

    bool ShaderResource::is_valid() const
    {
        if (m_program_id == 0) return false;

        glValidateProgram(m_program_id);
        GLint status;
        glGetProgramiv(m_program_id, GL_VALIDATE_STATUS, &status);
        return status == GL_TRUE;
    }

    // Uniform setting implementations
    void ShaderResource::set_uniform(const std::string& name, bool value)
    {
        set_uniform(name, static_cast<int>(value));
    }

    void ShaderResource::set_uniform(const std::string& name, int value)
    {
        if (!validate_uniform(name, UniformType::INT))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform1i(location, value);
        }
    }

    void ShaderResource::set_uniform(const std::string& name, float value)
    {
        if (!validate_uniform(name, UniformType::FLOAT))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform1f(location, value);
        }
    }

    void ShaderResource::set_uniform(const std::string& name, const glm::vec2& value)
    {
        if (!validate_uniform(name, UniformType::FLOAT_2))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform2f(location, value.x, value.y);
        }
    }

    void ShaderResource::set_uniform(const std::string& name, const glm::vec3& value)
    {
        if (!validate_uniform(name, UniformType::FLOAT_3))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform3f(location, value.x, value.y, value.z);
        }
    }

    void ShaderResource::set_uniform(const std::string& name, const glm::vec4& value)
    {
        if (!validate_uniform(name, UniformType::FLOAT_4))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform4f(location, value.x, value.y, value.z, value.w);
        }
    }

    void ShaderResource::set_uniform(const std::string& name, const glm::mat3& value)
    {
        if (!validate_uniform(name, UniformType::MATRIX_3))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void ShaderResource::set_uniform(const std::string& name, const glm::mat4& value)
    {
        if (!validate_uniform(name, UniformType::MATRIX_4))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
        }
    }

    void ShaderResource::set_uniform(const std::string &name, const std::vector<float> &values)
    {
        if (!validate_uniform(name, UniformType::FLOAT))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform1fv(location, static_cast<GLsizei>(values.size()), values.data());
        }
    }

    void ShaderResource::set_uniform(const std::string &name, const std::vector<glm::vec3> &values)
    {
        if (!validate_uniform(name, UniformType::FLOAT_3))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniform3fv(location, static_cast<GLsizei>(values.size()), glm::value_ptr(values[0]));
        }
    }

    void ShaderResource::set_uniform(const std::string &name, const std::vector<glm::mat4> &values)
    {
        if (!validate_uniform(name, UniformType::MATRIX_4))
            return;

        GLint location = get_uniform_location(name);
        if (location != -1)
        {
            glUniformMatrix4fv(location, static_cast<GLsizei>(values.size()), GL_FALSE, glm::value_ptr(values[0]));
        }
    }

    GLint ShaderResource::get_uniform_location(const std::string& name)
    {
        auto it = m_uniforms.find(name);
        if (it != m_uniforms.end())
        {
            return it->second.location;
        }

        // Try to find it dynamically
        GLint location = glGetUniformLocation(m_program_id, name.c_str());
        if (location != -1)
        {
            m_uniforms[name] = UniformInfo{location, UniformType::FLOAT, name};
        }

        return location;
    }

    bool ShaderResource::reload()
    {
        unload();
        return load();
    }

    bool ShaderResource::load_single_file()
    {
        try
        {
            if (!m_file_system.exists(m_path))
            {
                LOG_ERROR("ShaderResource file not found: {}", m_path.string());
                return false;
            }

            auto source = m_file_system.read_text(m_path);
            if (!source)
            {
                LOG_ERROR("failed to read file {}", m_path.string());
                return false;
            }

            m_memory_usage = source->size();

            if (!m_path.has_extension())
            {
                LOG_ERROR("Shader file has no extension: {}", m_path.string());
                return false;
            }
        
            std::string extension = m_path.extension().string();
            std::transform(
                extension.begin(), 
                extension.end(), 
                extension.begin(),
               [](unsigned char c){ return std::tolower(c); }
            );

            ShaderType type = get_shader_type_from_extension(extension);

            if (type == ShaderType::VERTEX || type == ShaderType::FRAGMENT)
            {
                // For single file, we assume it contains both vertex and fragment shaders
                // separated by specific markers or we create a simple program with one shader
                GLuint shader_id = 0;
                if (!compile_shader(*source, type, shader_id))
                    return false;

                m_program_id = glCreateProgram();
                glAttachShader(m_program_id, shader_id);
                m_shader_ids.push_back(shader_id);

                return link_program();
            }
            else
            {
                LOG_ERROR("ShaderResource unsupported shader type for single file: {}", m_path.string());
                return false;
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to load shader from file: {} - {}", m_path.string(), e.what());
            return false;
        }
    }

    bool ShaderResource::load_separate_files()
    {
        try
        {
            auto files = m_file_system.list_directory(m_path, false);

            m_program_id = glCreateProgram();
            if (m_program_id == 0)
            {
                LOG_ERROR("ShaderResource failed to create program");
                return false;
            }

            bool has_essential_shaders = false;

            for (const auto& file : files)
            {
                if (!file.has_extension())
                {
                    LOG_WARNING("Shader file has no extension: {}", file.string());
                    continue;
                }

                std::string extension = file.extension().string();
                std::transform(
                    extension.begin(), 
                    extension.end(), 
                    extension.begin(),
                    [](unsigned char c){ return std::tolower(c); }
                );

                ShaderType type = get_shader_type_from_extension(extension);

                if (type == ShaderType::VERTEX || type == ShaderType::FRAGMENT)
                {
                    has_essential_shaders = true;
                }

                auto source = m_file_system.read_text(file);
                if (!source)
                {
                    LOG_WARNING("Failed to read file {}", file.string());
                    continue;
                }

                m_memory_usage += source->size();

                GLuint shader_id = 0;
                if (compile_shader(*source, type, shader_id))
                {
                    m_shader_ids.push_back(shader_id);
                    glAttachShader(m_program_id, shader_id);
                    m_shader_types.push_back(type);
                }
                else
                {
                    LOG_ERROR("ShaderResource failed to compile shader: {}", file.string());
                    return false;
                }
            }

            if (!has_essential_shaders)
            {
                LOG_ERROR("ShaderResource missing essential shaders (vertex/fragment) in: {}", m_path.string());
                return false;
            }

            return link_program();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to load shader from directory: {} - {}", m_path.string(), e.what());
            return false;
        }
    }

    bool ShaderResource::compile_shader(const std::string &source,
                                        const ShaderType type,
                                        GLuint &shader_id)
    {
        GLuint gl_shader_type {0};
        switch (type)
        {
            case ShaderType::VERTEX:
                gl_shader_type = GL_VERTEX_SHADER;
                break;
            case ShaderType::FRAGMENT:
                gl_shader_type = GL_FRAGMENT_SHADER;
                break;
            case ShaderType::GEOMETRY:
                gl_shader_type = GL_GEOMETRY_SHADER;
                break;
            case ShaderType::TESSELLATION_CONTROL:
                gl_shader_type = GL_TESS_CONTROL_SHADER;
                break;
            case ShaderType::TESSELLATION_EVALUATION:
                gl_shader_type = GL_TESS_EVALUATION_SHADER;
                break;
            case ShaderType::COMPUTE:
                gl_shader_type = GL_COMPUTE_SHADER;
                break;

            default:
                return false;
        }

        shader_id = glCreateShader(gl_shader_type);
        if (shader_id == 0)
        {
            m_log += "Failed to create shader\n";
            return false;
        }

        const char* source_c_str = source.c_str();
        glShaderSource(shader_id, 1, &source_c_str, nullptr);
        glCompileShader(shader_id);

        GLint success;
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            GLchar info_log[1024];
            glGetShaderInfoLog(shader_id, sizeof(info_log), nullptr, info_log);
            m_log += "Shader compilation failed:\n" + std::string(info_log) + "\n";
            glDeleteShader(shader_id);
            shader_id = 0;
            return false;
        }

        return true;
    }

    bool ShaderResource::link_program()
    {
        if (m_program_id == 0)
        {
            m_program_id = glCreateProgram();
        }

        glLinkProgram(m_program_id);

        GLint success;
        glGetProgramiv(m_program_id, GL_LINK_STATUS, &success);
        if (!success)
        {
            GLchar info_log[1024];
            glGetProgramInfoLog(m_program_id, sizeof(info_log), nullptr, info_log);
            m_log += "Program linking failed:\n" + std::string(info_log) + "\n";
            return false;
        }

        return true;
    }

    const ShaderResource::UniformInfo* ShaderResource::find_uniform(
        const std::string& name) const
    {
        const auto it = m_uniforms.find(name);
        return (it != m_uniforms.end()) ? &it->second : nullptr;
    }

    bool ShaderResource::validate_uniform(
        const std::string& name,
        const UniformType expected) const
    {
        const auto* uniform = find_uniform(name);

        if (!uniform)
        {
            LOG_ERROR(
                "Shader '{}' uniform '{}' not found",
                m_path.string(), name
            );
            return false;
        }

        if (uniform->type != expected)
        {
            LOG_ERROR(
                "Shader '{}' uniform '{}' type mismatch. Expected {}, got {}",
                m_path.string(),
                name,
                uniform_type_to_string(expected),
                uniform_type_to_string(uniform->type)
            );
            return false;
        }

        return true;
    }

    void ShaderResource::detect_uniforms()
    {
        m_uniforms.clear();

        GLint uniform_count;
        glGetProgramiv(m_program_id, GL_ACTIVE_UNIFORMS, &uniform_count);

        GLchar uniform_name[256];
        for (GLint i = 0; i < uniform_count; ++i)
        {
            GLsizei length;
            GLint size;
            GLenum type;

            glGetActiveUniform(
                m_program_id,
                static_cast<GLuint>(i),
                sizeof(uniform_name),
                &length,
                &size,
                &type,
                uniform_name);

            GLint location = glGetUniformLocation(m_program_id, uniform_name);
            if (location != -1)
            {
                UniformInfo uniform;
                uniform.location = location;
                uniform.name = uniform_name;

                switch (type)
                {
                    case GL_FLOAT:
                        uniform.type = UniformType::FLOAT;
                        break;
                    case GL_FLOAT_VEC2:
                        uniform.type = UniformType::FLOAT_2;
                        break;
                    case GL_FLOAT_VEC3:
                        uniform.type = UniformType::FLOAT_3;
                        break;
                    case GL_FLOAT_VEC4:
                        uniform.type = UniformType::FLOAT_4;
                        break;
                    case GL_INT:
                        uniform.type = UniformType::INT;
                        break;
                    case GL_INT_VEC2:
                        uniform.type = UniformType::INT_2;
                        break;
                    case GL_INT_VEC3:
                        uniform.type = UniformType::INT_3;
                        break;
                    case GL_INT_VEC4:
                        uniform.type = UniformType::INT_4;
                        break;
                    case GL_UNSIGNED_INT:
                        uniform.type = UniformType::UINT;
                        break;
                    case GL_UNSIGNED_INT_VEC2:
                        uniform.type = UniformType::UINT_2;
                        break;
                    case GL_UNSIGNED_INT_VEC3:
                        uniform.type = UniformType::UINT_3;
                        break;
                    case GL_UNSIGNED_INT_VEC4:
                        uniform.type = UniformType::UINT_4;
                        break;
                    case GL_FLOAT_MAT3:
                        uniform.type = UniformType::MATRIX_3;
                        break;
                    case GL_FLOAT_MAT4:
                        uniform.type = UniformType::MATRIX_4;
                        break;
                    case GL_SAMPLER_2D:
                        uniform.type = UniformType::SAMPLER_2D;
                        break;
                    case GL_SAMPLER_CUBE:
                        uniform.type = UniformType::SAMPLER_CUBE;
                        break;

                    default:
                        uniform.type = UniformType::FLOAT;
                        break;
                }

                m_uniforms[uniform.name] = uniform;
            }
        }
    }

    void ShaderResource::cleanup_shaders()
    {
        for (const GLuint shader_id : m_shader_ids)
        {
            if (shader_id != 0)
            {
                glDeleteShader(shader_id);
            }
        }
        m_shader_ids.clear();
    }

    std::string ShaderResource::get_shader_extension(ShaderType type) const
    {
        switch (type)
        {
            case ShaderType::VERTEX:                    return ".vert";
            case ShaderType::FRAGMENT:                  return ".frag";
            case ShaderType::GEOMETRY:                  return ".geom";
            case ShaderType::TESSELLATION_CONTROL:      return ".tesc";
            case ShaderType::TESSELLATION_EVALUATION:   return ".tese";
            case ShaderType::COMPUTE:                   return ".comp";

            default:                                    return "";
        }
    }


    ShaderType ShaderResource::get_shader_type_from_extension(const std::string& extension) const
    {
        if (extension == ".vert") return ShaderType::VERTEX;
        if (extension == ".frag") return ShaderType::FRAGMENT;
        if (extension == ".geom") return ShaderType::GEOMETRY;
        if (extension == ".tesc") return ShaderType::TESSELLATION_CONTROL;
        if (extension == ".tese") return ShaderType::TESSELLATION_EVALUATION;
        if (extension == ".comp") return ShaderType::COMPUTE;
        if (extension == ".glsl") return ShaderType::VERTEX; // Default for .glsl files

        LOG_ERROR("Unsupported shader extension: {}", extension);
        return ShaderType::UNKNOWN;
    }

    std::chrono::steady_clock::time_point ShaderResource::get_last_access_time() const
    {
        return m_last_access_time;
    }

} // namespace c2l::core::resources