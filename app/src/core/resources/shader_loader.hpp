//
// Created by Akhmad on 11/4/25.
//

#ifndef CODE2LOGIC_SHADER_LOADER_HPP
#define CODE2LOGIC_SHADER_LOADER_HPP


#include "core/resources/i_resource_loader.hpp"
#include "core/resources/shader_resource.hpp"
#include "core/file_system/i_file_system.hpp"

#include <string>
#include <vector>
#include <filesystem>

namespace c2l::core::resources
{
    /**
     * @brief Shader resource loader for OpenGL shaders
     */
    class ShaderLoader final : public IResourceLoader
    {
    public:
        ShaderLoader() = default;

        ShaderLoader(const ShaderLoader&) = delete;
	    ShaderLoader& operator=(const ShaderLoader&) = delete;
        
        /**
         * @brief Checks whether the loader supports the given file extension
         *
         * @param extension File extension including dot(e.g. ".vert", ".frag", ".geom", etc.)
         * @return True if extension supports, false otherwise
         */
        [[nodiscard]] bool can_load(const std::string& extension) const override
        {
            static const std::vector<std::string> supported_extensions =
            {
                ".vert", ".frag", ".geom", ".tesc", ".tese", ".comp", ".glsl"
            };

            for (const auto& ext : supported_extensions)
            {
                if (extension == ext)
                {
                    return true;
                }
            }

            // Also support directories (for separate shader files)
            return extension.empty(); // Directories have no extension
        }

        /**
         * @param path shader path to load
         * @param file_system Resolves the path using IFileSystem class
         * @return Returns std::shared_ptr<IResource> of the loaded shader
         */
        std::shared_ptr<IResource> load(
            const std::filesystem::path& path,
            core::filesystem::IFileSystem& file_system) override
        {
            auto shader = std::make_shared<ShaderResource>(path, file_system);
            if (shader->load())
            {
                return shader;
            }
            return nullptr;
        }

        /**
         * @return std::vector<std::string> of supported shader extensions
         */
        [[nodiscard]] std::vector<std::string> get_supported_extensions() const override
        {
            return {
                ".vert", ".frag", ".geom", ".tesc", ".tese", ".comp", ".glsl"
            };
        }
    };

} // namespace c2l::core::resources

#endif //CODE2LOGIC_SHADER_LOADER_HPP