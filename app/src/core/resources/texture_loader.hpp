#ifndef CORE_RESOURCES_TEXTURE_LOADER_HPP
#define CORE_RESOURCES_TEXTURE_LOADER_HPP

#include "core/resources/i_resource_loader.hpp"
#include "core/resources/i_resource.hpp"
#include "core/resources/texture_resource.hpp"
#include "core/file_system/i_file_system.hpp"

#include <string>
#include <vector>

namespace c2l::core::resources
{
    /**
     * @brief Texture resource loader for various image formats
     */
    class TextureLoader final : public core::resources::IResourceLoader
    {
    public:
        TextureLoader() = default;

        /**
         * @brief Checks whether the loader supports the given file extension.
         *
         * @param extension File extension including loading dot (e.g. ".png", "jpg", and ...etc.)
         * @return True if extension is supported, false otherwise
         */
        [[nodiscard]] bool can_load(const std::string& extension) const override
        {
            static const std::vector<std::string> supported_extensions = {
                ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", 
                ".tiff", ".tif", ".gif", ".hdr", ".pic"
            };
            
            for (const auto& ext : supported_extensions) 
            {
                if (extension == ext) 
                {
                    return true;
                }
            }
            return false;
        }

        /**
         *
         * @param path Texture path to load
         * @param file_system Resolves the path using IFileSystem
         * @return Return std::shared_ptr<IResource> of the loaded texture
         */
        std::shared_ptr<core::resources::IResource> load(
            const std::string& path, 
            core::filesystem::IFileSystem& file_system) override
        {
            auto texture = std::make_shared<TextureResource>(path, file_system);
            if (texture->load_data_only())
            {
                return texture;
            }
            return nullptr;
        }

        /**
         * @return std::vector<std::string> of supported texture extensions
         */
        [[nodiscard]] std::vector<std::string> get_supported_extensions() const override
        {
            return {
                ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".psd", 
                ".tiff", ".tif", ".gif", ".hdr", ".pic"
            };
        }
    };
    
} // namespace c2l::core::resources

#endif // CORE_RESOURCES_TEXTURE_LOADER_HPP