#ifndef I_RESOURCE_HPP
#define I_RESOURCE_HPP

#include <string>
#include <chrono>

namespace c2l::core::resources
{
	/**
	 * @brief Resource loading state
	 */
	enum class ResourceState
	{
		Unloaded,
		Loading,
		Loaded,
		Error
	};

	/**
	 * @brief Base interface for all loadable resources
	 * 
	 * Resources can be textures, fonts, shaders, meshes, etc.
	 * This interface provides common lifecycle management.
	 */
	class IResource
	{
	public:
		virtual ~IResource() = default;

		/**
		 * @brief Load the resource from disk
		 * @return true if successful, false otherwise
		 */
		virtual bool load() = 0;

		/**
		 * @brief Unload the resource and free memory
		 */
		virtual void unload() = 0;

		/**
		 * @brief Update last access time
		 */
		virtual void update_access_time() = 0;

		/**
		 * @brief Get the resource path
		 * @return Resource file path
		 */
		[[nodiscard]] virtual const std::string& get_path() const = 0;

		/**
		 * @brief Get the current resource state
		 * @return Current resource state
		 */
		[[nodiscard]] virtual ResourceState get_state() const = 0;

		/** 
		 * @brief Check if Resource is loaded
		 * @return true if successful, false otherwise
		 */
		[[nodiscard]] virtual bool is_loaded() const = 0;

	    /**
	     * @brief Get memory usage of the resource in bytes
	     * @return Memory usage in bytes
	     */
	    [[nodiscard]] virtual uint64_t get_memory_usage() const = 0;
	    
	    /**
	     * @brief Get last access time for LRU caching
	     * @return Last access time
	     */
	    [[nodiscard]] virtual std::chrono::steady_clock::time_point get_last_access_time() const = 0;
	};

} // namespace c2l::core::resources

#endif // I_RESOURCE_HPP