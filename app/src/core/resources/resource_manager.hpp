#ifndef RESOURCE_MANAGER_HPP
#define RESOURCE_MANAGER_HPP

#include "core/resources/i_resource.hpp"
#include "core/resources/i_resource_loader.hpp"
#include "core/file_system/i_file_system.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <optional>


namespace c2l::core::resources
{
	/**
	 * @brief Main resource manager for loading and caching resources
	 * 
	 * Features:
	 * - Automatic resource caching 
	 * - Memory management and LRU eviction
	 * - Hot-reloading support
	 * - Thread-safe operations
	 * - Extensible loader system
	 * - Cross-platform path resolution
	 */
	class ResourceManager
	{
	public:
		/**
		 * @brief Construct a resource manager
		 * @param file_system File system implementation to use
		 * @param thread_manager To handle concurrency
		 */
		ResourceManager(
			filesystem::IFileSystem& file_system,
			c2l::core::ThreadManager& thread_manager);

		~ResourceManager();

		/**
		 * @brief Load a resource of specific type
		 * @tparam T Resource type (must derive from IResource)
		 * @param path Resource path (relative or absolute)
		 * @return Shared pointer to loaded resource, nullptr if failed
		 * 
		 * @example 
		 * auto font resource_manager->load<FontResource>("font/Roboto.ttf");
		 * auto texture resource_manager->load<TextureResource>("texture/icon.png");
		 */
		template<typename T>
		std::shared_ptr<T> load(const std::filesystem::path& path);

		/**
		* @brief Get a previously loaded resource
		* @tparam T Resource type
		* @param path Resource path
		* @return Shared pointer to resource if loaded, nullptr otherwise
		*/
		template<typename T>
		std::shared_ptr<T> get(const std::filesystem::path& path);

		/**
		 * @brief Unload a specific resource
		 * @param path Resource path to unload
		 * @return true if found and unloaded, false otherwise
		 */
		bool unload(const std::filesystem::path& path);

		/**
	     * @brief Unload all resources not used since specified time
	     * @param older_than Unload resources last accessed before this time
	     * @return Number of resources unloaded
	     */
	    size_t unload_unused(const std::chrono::steady_clock::time_point& older_than);

	    /**
	     * @brief Unload all resources not used since an hour ago
	     * @return Number of resources unloaded
	     */
	    size_t unload_unused();

	    /**
	     * @brief Unload all resources
	     */
	    void unload_all();

	    /**
	     * @brief Set maximum memory budget in bytes
	     * @param max_memory Maximum memory in bytes
	     */
	    void set_memory_budget(uint64_t max_memory);

	    /**
	     * @brief Get current memory usage in bytes
	     * @return Total memory usage by all resources
	     */
	    uint64_t get_memory_usage() const;

	    /**
	     * @brief Get member of loaded resources
	     * @return Count of loaded resources
	     */
	    size_t get_resource_count() const;

	    /**
	     * @brief Perform garbage collection (unload unused resources)
	     * @return Number of resources unloaded
	     */
	    size_t garbage_collect();

	    /**
	     * @brief Register a resource loader
	     * @param loader Unique pointer to loader instance
	     */
	    void register_loader(std::unique_ptr<IResourceLoader> loader);

	    /**
	     * @brief Unregister all loaders for a specific extension
	     * @param extension File extension to remove loaders for
	     */
	    void unregister_loader(const std::string& extension);

	    /**
	     * @brief Enable or disable hot reloading 
	     * @param enable true to enable, false to disable
	     */
	    void enable_hot_reloading(bool enable);

	    /**
	     * @brief Check for changed files and reloaded them
	     * @return Number of resources reloaded
	     * 
	     * @note Call this periodically (e.g., once per frame) for hot reloading
	     */
	    size_t check_for_changes();

	    /**
	     * @brief Check if a resource exists
	     * @param path Resource path to check
	     * @return true if exists, false otherwise
	     */
	    bool resource_exists(const std::filesystem::path& path) const;

	    /**
	     * @brief Get all loaded resource paths
	     * @return Vector of loaded resource paths
	     */
	    std::vector<std::filesystem::path> get_loaded_resources() const;

	    /**
	     * @brief Preload multiple resources 
	     * @param path Vector of resource path to preload
	     */
	    void preload_resources(const std::vector<std::filesystem::path>& path);

	    /**
	     * @brief Scan directory and register found resources
	     * @param path Directory path to scan
	     * @param recursive Whether to scan subdirectories
	     */
	    void scan_directory(const std::filesystem::path& path, bool recursive = true);
	    
	    /**
	     * @brief Get the resolved absolute path for a resource
	     * @param relative_path Relative resource path
	     * @return Absolute path if found, std::nullopt if not found
	     */
	    [[nodiscard]] std::optional<std::filesystem::path> 
		get_resource_path(const std::filesystem::path& relative_path) const;

		/**
		 * @return Returns reference of filesystem::IFileSystem&
		 */
		[[nodiscard]] filesystem::IFileSystem& get_file_system() const;

	private:
		struct ResourceEntry
		{
			std::shared_ptr<IResource> resource;
			std::chrono::steady_clock::time_point last_access;
			std::chrono::system_clock::time_point file_last_modified;
			uint64_t memory_usage;
		};

		std::shared_ptr<IResource> load_internal(const std::filesystem::path& path);
		IResourceLoader* find_loader(const std::filesystem::path& path) const;
		void evict_resources_if_needed();
		std::string get_extension(const std::filesystem::path& path) const;
		void update_resource_entry(
			const std::filesystem::path& path, 
			std::shared_ptr<IResource> resource);

		// Members
		filesystem::IFileSystem& m_file_system;
		c2l::core::ThreadManager& m_thread_manager;

		std::unordered_map<std::filesystem::path, ResourceEntry> m_resources;
		std::vector<std::unique_ptr<IResourceLoader>> m_loaders;
		mutable std::mutex m_mutex;
		std::atomic<uint64_t> m_memory_usage	{0};
	    std::atomic<uint64_t> m_memory_budget	{0};
	    std::atomic<bool> m_hot_reload_enabled	{false};
	};

	template<typename T> 
	std::shared_ptr<T> ResourceManager::load(const std::filesystem::path& path)
	{
		static_assert(std::is_base_of_v<IResource, T>, 
					  "T must derive from IResource");

		auto resource = load_internal(path);
		return std::dynamic_pointer_cast<T>(resource);
	}

	template<typename T>
	std::shared_ptr<T> ResourceManager::get(const std::filesystem::path& path) 
	{
		static_assert(std::is_base_of_v<IResource, T>, 
					  "T must derive from IResource");

		std::lock_guard<std::mutex>lock(m_mutex);
		auto it = m_resources.find(path);
		if (it != m_resources.end() && it->second.resource->is_loaded())
		{
			it->second.resource->update_access_time();
	        it->second.last_access = std::chrono::steady_clock::now();
	        return std::dynamic_pointer_cast<T>(it->second.resource);
		}

		return nullptr;
	}

} // namespace c2l::core::resources

#endif // RESOURCE_MANAGER_HPP