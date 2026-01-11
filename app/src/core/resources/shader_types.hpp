//
// Created by Akhmad on 1/6/26.
//

#ifndef CODE2LOGIC_SHADER_TYPES_HPP
#define CODE2LOGIC_SHADER_TYPES_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace c2l::core::resources
{
    /**
     * @brief Shader types supported by the system
     */
    enum class ShaderType : uint8_t
    {
        VERTEX,
        FRAGMENT,
        GEOMETRY,
        TESSELLATION_CONTROL,
        TESSELLATION_EVALUATION,
        COMPUTE,

        UNKNOWN
    };

    /**
     * @brief Uniform variable types
     */
    enum class UniformType : uint8_t
    {
        FLOAT,
        FLOAT_2,
        FLOAT_3,
        FLOAT_4,
        INT,
        INT_2,
        INT_3,
        INT_4,
        UINT,
        UINT_2,
        UINT_3,
        UINT_4,
        MATRIX_3,
        MATRIX_4,
        SAMPLER_2D,
        SAMPLER_CUBE
    };

    inline const char* shader_type_to_string(ShaderType type)
    {
        switch (type)
        {
            case ShaderType::VERTEX:                    return "Vertex";
            case ShaderType::FRAGMENT:                  return "Fragment";
            case ShaderType::GEOMETRY:                  return "Geometry";
            case ShaderType::TESSELLATION_CONTROL:      return "Tessellation Control";
            case ShaderType::TESSELLATION_EVALUATION:   return "Tessellation Evaluation";
            case ShaderType::COMPUTE:                   return "Compute";
            default:                                    return "Unknown";
        }
    }

    inline ShaderType shader_string_to_type(const std::string& type)
    {
        if (type == "Vertex")                   return ShaderType::VERTEX;
        if (type == "Fragment")                 return ShaderType::FRAGMENT;
        if (type == "Geometry")                 return ShaderType::GEOMETRY;
        if (type == "Tessellation Control")     return ShaderType::TESSELLATION_CONTROL;
        if (type == "Tessellation Evaluation")  return ShaderType::TESSELLATION_EVALUATION;
        if (type == "Compute")                  return ShaderType::COMPUTE;

        return ShaderType::UNKNOWN;
    }

    inline const char* uniform_type_to_string(UniformType type)
    {
        switch (type)
        {
            case UniformType::FLOAT:        return "float";
            case UniformType::FLOAT_2:      return "vec2";
            case UniformType::FLOAT_3:      return "vec3";
            case UniformType::FLOAT_4:      return "vec4";
            case UniformType::INT:          return "int";
            case UniformType::INT_2:        return "ivec2";
            case UniformType::INT_3:        return "ivec3";
            case UniformType::INT_4:        return "ivec4";
            case UniformType::UINT:         return "uint";
            case UniformType::UINT_2:       return "uvec2";
            case UniformType::UINT_3:       return "uvec3";
            case UniformType::UINT_4:       return "uvec4";
            case UniformType::MATRIX_3:     return "mat3";
            case UniformType::MATRIX_4:     return "mat4";
            case UniformType::SAMPLER_2D:   return "sampler2D";
            case UniformType::SAMPLER_CUBE: return "samplerCube";
            default:                        return "unknown";
        }
    }


} // namespace c2l::core::resources

#endif //CODE2LOGIC_SHADER_TYPES_HPP