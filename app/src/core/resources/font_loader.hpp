#ifndef FONT_LOADER_HPP
#define FONT_LOADER_HPP

#include "core/resources/i_resource_loader.hpp"
#include "core/resources/i_resource.hpp"
#include "core/resources/font_resource.hpp"
#include "core/file_system/i_file_system.hpp"

#include <string>
#include <vector>
#include <filesystem>

namespace c2l::core::resources
{
	/**
	 * @brief Font resource loader for TTF and OTF files
	 */
	class FontLoader final : public IResourceLoader
	{
	public:
		/**
	     * @brief Construct with optional default font size
	     * @param default_size Default font size in pixels
	     */
	    explicit FontLoader(const float default_size = 16.0f) : m_default_size(default_size) {}
		~FontLoader() override = default;

		FontLoader(const FontLoader&) = delete;
		FontLoader& operator=(const FontLoader&) = delete;

		/**
		 * @brief Checks whether the loader supports the given file extension.
		 *
		 * @param extension File extension including leading dot (e.g. ".ttf", ".otf").
		 * @return True if the extension is supported, false otherwise.
		 */
		[[nodiscard]] bool can_load(const std::string& extension) const override
	    {
	        return extension == ".ttf" || extension == ".otf";
	    }

		/**
		 * @param path Font path to load
		 * @param file_system Resolves the path using IFileSystem class
		 * @return Returns std::shared_ptr<IResource> of the loaded font
		 */
		std::shared_ptr<IResource> load(
	        const std::filesystem::path& path, 
	        core::filesystem::IFileSystem& file_system) override
	    {
	        auto font = std::make_shared<FontResource>(path, file_system, m_default_size);
	        if (font->load()) 
	        {
	            return font;
	        }
	        return nullptr;
	    }

		/**
		 * @return std::vector<std::string> of supported font extensions
		 */
		[[nodiscard]] std::vector<std::string> get_supported_extensions() const override
	    {
	        return {".ttf", ".otf"};
	    }

	private:
	    float m_default_size;
	};

} // namespace c2l::core::resources

#endif // FONT_LOADER_HPP