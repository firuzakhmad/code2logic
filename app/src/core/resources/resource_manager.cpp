#include "core/resources/resource_manager.hpp"

#include <algorithm>
#include <iostream>
#include <thread>

#include "font_loader.hpp"
#include "shader_loader.hpp"
#include "texture_loader.hpp"
#include "core/utils/assert/assert.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::core::resources
{
	ResourceManager::ResourceManager(
		filesystem::IFileSystem& file_system,
		c2l::core::ThreadManager& thread_manager)
		: m_file_system{file_system}
		, m_thread_manager{thread_manager}
	{
		register_loader(std::make_unique<FontLoader>());
		register_loader(std::make_unique<TextureLoader>());
		register_loader(std::make_unique<ShaderLoader>());

		// TODO: Create default resource file (.json)
		// std::vector<std::filesystem::path> default_resources = {
		// 	// Fonts
		// 	"resources/fonts/roboto/Roboto-Black.ttf",
		// };
		// preload_resources(default_resources);

	}

	ResourceManager::~ResourceManager()
	{
		unload_all();
	}

	std::shared_ptr<IResource> ResourceManager::load_internal(const std::filesystem::path& path)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// Checking if already loaded
		auto it = m_resources.find(path);
		if (it != m_resources.end() && it->second.resource->is_loaded())
		{
			it->second.last_access = std::chrono::steady_clock::now();
			it->second.resource->update_access_time();
			return it->second.resource;
		}

		// Resolving path using file system
	    auto resolved_path = m_file_system.resolve_path(path);
		if (!resolved_path)
	    {
		    // Try with original path as absolute
		    resolved_path = path;
		    if (!m_file_system.exists(*resolved_path))
	        {
		    	LOG_ERROR("Failed to find: {}", resolved_path->string());
	            return nullptr;
	        }
	    }

	    // Finding appropriate loader
	    IResourceLoader* loader = find_loader(resolved_path->string());
	    if (!loader)
	    {
		    LOG_ERROR("Failed to find a loader for file: {}", resolved_path->string());
	    	return nullptr;
	    }

	    // Loading the resource
	    auto resource = loader->load(*resolved_path, m_file_system);
	    if (!resource || !resource->is_loaded())
	    {
	    	LOG_ERROR("Failed to load file: {}", resolved_path->string());
	    	return nullptr;
	    }

	    update_resource_entry(resolved_path->string(), resource);

	    // Checking memory constraints
	    evict_resources_if_needed();

	    return resource;
	}

	IResourceLoader* ResourceManager::find_loader(const std::filesystem::path& path) const
	{
		// First check if it's a directory
		auto stats = m_file_system.get_file_stats(path);
		if (stats.is_directory)
		{
			// Look for loaders that can handle directories (empty extension)
			for (const auto& loader : m_loaders)
			{
				if (loader->can_load("")) // Directories have no extension
				{
					return loader.get();
				}
			}
		}

		// If not a directory, check by file extension
		std::string extension = get_extension(path);

		for (const auto& loader : m_loaders)
		{
			if (loader->can_load(extension))
			{
				return loader.get();
			}
		}

		return nullptr;
	}

	void ResourceManager::evict_resources_if_needed()
	{
		if (m_memory_budget == 0 || m_memory_usage <= m_memory_budget)
		{
			return;
		}

		// Converting to vector for sorting
		std::vector<std::pair<std::filesystem::path, ResourceEntry>> entries(m_resources.begin(), m_resources.end());

		// Sorting by last access time (oldest first)
		std::sort(
			entries.begin(),
			entries.end(),
			[](const auto& a, const auto& b) 
			{
				return a.second.last_access < b.second.last_access;
			}
		);

		// Evict oldest resources until under budget
		for (const auto& [path, entry] : entries)
		{
			if (m_memory_usage <= m_memory_budget)
			{
				break;
			}

			if (entry.resource.use_count() == 1)
			{
				unload(path);
			}
		}
	}

	std::string ResourceManager::get_extension(const std::filesystem::path& path) const
	{
		
		if (!path.has_extension())
			return {};

		std::string ext = path.extension().string();

		std::transform(ext.begin(), ext.end(), ext.begin(),
					[](unsigned char c) { return std::tolower(c); });

		return ext;
	}

	void ResourceManager::update_resource_entry(
		const std::filesystem::path& path,
		std::shared_ptr<IResource> resource)
	{
		auto now = std::chrono::steady_clock::now();
		const auto file_stats = m_file_system.get_file_stats(path);

		ResourceEntry entry;
		entry.resource = std::move(resource);
		entry.last_access = now;
		entry.file_last_modified = file_stats.last_modified;
		entry.memory_usage = entry.resource->get_memory_usage();

		m_memory_usage += entry.memory_usage;
		m_resources[path] = std::move(entry);
	}

	bool ResourceManager::unload(const std::filesystem::path& path)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		auto it = m_resources.find(path);
		if (it != m_resources.end())
		{
			m_memory_usage -= it->second.memory_usage;
			it->second.resource->unload();
			m_resources.erase(it);
			return true;
		}

		return false;
	}

	size_t ResourceManager::unload_unused(
		const std::chrono::steady_clock::time_point& older_than)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		size_t unloaded_count = 0;
		auto it = m_resources.begin();

		while (it != m_resources.end())
		{
			if (it->second.last_access < older_than && it->second.resource.use_count() == 1)
			{
				m_memory_usage -= it->second.memory_usage;
				it->second.resource->unload();
				it = m_resources.erase(it);
				unloaded_count++;
			} else 
			{
				++it;
			}
		}

		return unloaded_count;
	} 

	size_t ResourceManager::unload_unused() 
	{
		auto one_hour_ago = std::chrono::steady_clock::now() - std::chrono::hours(1);
		return unload_unused(one_hour_ago);
	}

	void ResourceManager::unload_all()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		for (auto& [path, entry] : m_resources)
		{
			entry.resource->unload();
		}

		m_resources.clear();
		m_memory_usage = 0;
	}

	void ResourceManager::set_memory_budget(uint64_t max_memory)
	{
		m_memory_budget = max_memory;
		evict_resources_if_needed();
	}

	uint64_t ResourceManager::get_memory_usage() const
	{
		return m_memory_usage;
	}

	size_t ResourceManager::get_resource_count() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_resources.size();
	}

	size_t ResourceManager::garbage_collect()
	{
		auto five_minutes_ago = std::chrono::steady_clock::now() - std::chrono::minutes(5);
		return unload_unused(five_minutes_ago);
	}

	void ResourceManager::register_loader(std::unique_ptr<IResourceLoader> loader) 
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_loaders.push_back(std::move(loader));
	}

	void ResourceManager::unregister_loader(const std::string& extension)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		m_loaders.erase(
			std::remove_if(m_loaders.begin(), m_loaders.end(),
				[&extension](const auto& loader) 
				{
					return loader->can_load(extension);
				}),
			m_loaders.end()
		);
	}

	void ResourceManager::enable_hot_reloading(bool enable)
	{
		m_hot_reload_enabled = enable;
	}

	size_t ResourceManager::check_for_changes()
	{
		if (!m_hot_reload_enabled)
		{
			return 0;
		}

		std::lock_guard<std::mutex> lock(m_mutex);

		size_t reloaded_count = 0;

		for (auto& [path, entry] : m_resources)
		{
			auto current_stats = m_file_system.get_file_stats(path);

			if (current_stats.exists && current_stats.last_modified > entry.file_last_modified)
			{
				// File has changed, reload it
				if (entry.resource->load())
				{
					entry.file_last_modified = current_stats.last_modified;
					entry.memory_usage = entry.resource->get_memory_usage();
					reloaded_count++;
				}
			}
		}

		return reloaded_count;
	}

	bool ResourceManager::resource_exists(const std::filesystem::path& path) const
	{
		return m_file_system.exists(path) || m_file_system.resolve_path(path).has_value();
	}

	std::vector<std::filesystem::path> ResourceManager::get_loaded_resources() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		std::vector<std::filesystem::path> paths;
		paths.reserve(m_resources.size());

		for (const auto& [path, entry] : m_resources)
		{
			if (entry.resource->is_loaded())
			{
				paths.push_back(path);
			}
		}

		return paths;
	}

	void ResourceManager::preload_resources(
		const std::vector<std::string>& resources)
	{
		for (const auto& path : resources)
		{
			load_internal(path);
		}
	}

	void ResourceManager::scan_directory(
		const std::filesystem::path& path,
		bool recursive)
	{
	    auto files = m_file_system.list_directory(path, recursive);
	    
	    for (const auto& file : files) 
	    {
	        // Try to load each file with available loaders
	        load_internal(file);
	    }
	}

	std::optional<std::filesystem::path> ResourceManager::get_resource_path(
		const std::filesystem::path& relative_path) const
	{
		return m_file_system.resolve_path(relative_path);
	}

	filesystem::IFileSystem& ResourceManager::get_file_system() const
	{
		return m_file_system;
	}


} // namespace c2l::core::resources