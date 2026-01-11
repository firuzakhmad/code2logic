#ifndef I_RESOURCE_LOADER
#define I_RESOURCE_LOADER

#include "core/resources/i_resource.hpp"
#include "core/file_system/i_file_system.hpp"

#include <string>
#include <memory>
#include <vector>

namespace c2l::core::resources
{
	/**
	 * @brief Resource loader interface for specific file types
	 */ 
	class IResourceLoader
	{
	public:
		virtual ~IResourceLoader() = default;

		/**
		 * @brief Check if this loader can handle a file extension
		 * @param extension File extension (with dot, e.g., ".png")
		 * @return true if supported, false otherwise
		 */
		[[nodiscard]] virtual bool can_load(const std::string& extension) const = 0;

		/**
		 * @brief Load a resource from file
		 * @param path Path to the resource file
		 * @param file_system File system to use for loading
		 * @return Shared pointer to loaded resource, nullptr if failed
		 */
		virtual std::shared_ptr<IResource> load(const std::string& path,
												filesystem::IFileSystem& file_system) = 0;
		
		/**
		 * @brief Get support file extensions
		 * @return Vector of supported extensions (with dots)
		 */
		[[nodiscard]] virtual std::vector<std::string> get_supported_extensions() const = 0;
	};

} // namespace c2l::core::resources


#endif // I_RESOURCE_LOADER