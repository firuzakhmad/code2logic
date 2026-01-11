#include "std_file_system.hpp"
#include "core/utils/logger/logger.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>

namespace c2l::core::filesystem
{
	namespace fs = std::filesystem;
	
	StdFileSystem::StdFileSystem()
	{
		m_base_path = find_root_path();

		setup_default_search_path();
	}

	StdFileSystem::StdFileSystem(const std::string& base_path)
	{
		set_base_path(base_path);

		setup_default_search_path();
	}

	bool StdFileSystem::exists(const std::string& path) const
	{
		return fs::exists(path);
	}

	std::string StdFileSystem::read_text(const std::string& path) const
	{
		try
		{
			const auto resolved_path = resolve_path(path);
			if (resolved_path.empty())
				return {};

			std::ifstream file(resolved_path);
			if (!file)
				return {};

			std::stringstream buffer;
			buffer << file.rdbuf();
			return buffer.str();
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Exception while reading file '{}': {}", path, e.what());
			return {};
		}
	}

	std::vector<uint8_t> StdFileSystem::read_binary(const std::string& path) const
	{
		const auto resolved_path = resolve_path(path);
		if (resolved_path.empty())
		{
			LOG_ERROR("File not found: {}", path);
			return {};
		}

		std::ifstream file(resolved_path, std::ios::binary | std::ios::ate);
		if (!file)
		{
			LOG_ERROR("Cannot open file: {}", resolved_path);
			return {};
		}

		const std::streamsize size = file.tellg();
		if (size <= 0)
		{
			LOG_ERROR("Invalid file size: {}", resolved_path);
			return {};
		}

		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
		{
			LOG_ERROR("Failed to read file: {}", resolved_path);
			return {};
		}

		return buffer;
	}

	bool StdFileSystem::write_text(const std::string& path, const std::string& content) const
	{
		try
		{
			auto dir = fs::path(path).parent_path();
			if (!dir.empty() && !fs::exists(dir))
			{
				fs::create_directories(dir);
			}

			std::ofstream file(path);
			if (!file.is_open()) return false;

			file << content;
			return true;
		} catch (...)
		{
			return false;
		}
	}

	bool StdFileSystem::write_binary(const std::string& path, const std::vector<uint8_t>& data)
	{
		try
		{
			auto dir = fs::path(path).parent_path();
			if (!dir.empty() && !fs::exists(dir))
			{
				fs::create_directories(dir);
			}

			std::ofstream file(path, std::ios::binary);
        	if (!file.is_open()) return false;

        	file.write(reinterpret_cast<const char*>(data.data()), data.size());
        	return true;

		} catch(...)
		{
			return false;
		}
	}

	bool StdFileSystem::delete_file(const std::string& path)
	{
		try
		{
			return fs::remove_all(path) > 0;
		} catch(...)
		{
			return false;
		}
	}

	bool StdFileSystem::create_directory(const std::string& path)
	{
		try
		{
			return fs::create_directories(path);
		} catch(...)
		{
			return false;
		}
	}

	std::vector<std::string> StdFileSystem::list_directory(const std::string& path, bool recursive) const
	{
		std::vector<std::string> result;
		try
		{
			if (recursive)
			{
				for (const auto& entry : fs::recursive_directory_iterator(path))
				{
					result.push_back(entry.path().string());
				}
			} else {
				for (const auto& entry : fs::directory_iterator(path))
				{
					result.push_back(entry.path().string());
				}
			}
		} catch(...)
		{}

		return result;
	}

	std::string StdFileSystem::get_absolute_path(const std::string& path) const 
	{
	    try {
	        return fs::absolute(path).string();
	    } catch (...) {
	        return path;
	    }
	}

	std::string StdFileSystem::get_working_directory() const 
	{
	    return fs::current_path().string();
	}

	core::filesystem::FileStats StdFileSystem::get_file_stats(const std::string& path) const 
	{
		core::filesystem::FileStats stats{};
		try {
			if (fs::exists(path)) {
				auto fileStatus = fs::status(path);
				stats.exists = true;
				stats.is_directory = fs::is_directory(path);
				
				if (!stats.is_directory) {
					stats.size = fs::file_size(path);
				}
				
				auto ftime = fs::last_write_time(path);
				auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ftime - fs::file_time_type::clock::now()
					+ std::chrono::system_clock::now());
				stats.last_modified = sctp;
			}
		} catch (...) {
			// Leaving stats as default (exists = false)
		}
		return stats;
	}


	uint64_t StdFileSystem::get_file_size(const std::string& path) const 
	{
	    try {
	        return fs::file_size(path);
	    } catch (...) {
	        return 0;
	    }
	}

	std::chrono::system_clock::time_point StdFileSystem::get_last_modified(const std::string& path) const
	{
		try {
			auto ftime = fs::last_write_time(path);
			return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
				ftime - fs::file_time_type::clock::now()
				+ std::chrono::system_clock::now());
		} catch (...) {
			return std::chrono::system_clock::time_point{};
		}
	}


	void StdFileSystem::add_search_path(const std::string& path)
	{
		m_search_paths.insert(normalize_path(get_absolute_path(path)));
	}

	void StdFileSystem::remove_search_path(const std::string& path)
	{
		m_search_paths.erase(get_absolute_path(path));
	}

	std::string StdFileSystem::resolve_path(const std::string& relative_path) const 
	{
		// Checking if path is already absolute
	    fs::path path(relative_path);
	    if (path.is_absolute()) {
	        return fs::exists(path) ? path.string() : "";
	    }

	    // before base + relative:
		fs::path bp = fs::path(m_base_path) / path;

	    // Checking in base path first
	    fs::path base_path(m_base_path);
	    base_path /= path;
	    if (fs::exists(base_path))
	    {
	    	return base_path.string();
	    } 

	    // then inside the loop of search paths:
		for (const auto& search_path : m_search_paths) {
		    fs::path spath(search_path);
		    spath /= path;
		    if (fs::exists(spath)) {
		        return spath.string();
		    }
		}
	    // Not found
		LOG_ERROR("Path {} does not exist", relative_path);
	    return "";
	}

	void StdFileSystem::set_base_path(const std::string& path)
	{
		m_base_path = get_absolute_path(path);
	}

	std::string StdFileSystem::get_base_path() const 
	{
		return m_base_path;
	}

    std::string StdFileSystem::find_file_recursive(const std::string& start_dir, 
    											   const std::string& filename) const
    {
    	auto files = list_directory(start_dir, true);
    	for (const auto& file : files)
    	{
    		size_t last_slash = file.find_last_of("/\\");
            std::string current_filename = (last_slash == std::string::npos) 
                ? file 
                : file.substr(last_slash + 1);
                
            if (current_filename == filename) 
            {
                return file;
            }
    	}

		LOG_ERROR("Failed to find file — {}", filename);
    	return "";
    }


	std::string StdFileSystem::find_in_search_paths(const std::string& filename) const
	{
		if (filename.empty())
			return {};

		for (const auto& base_path : m_search_paths)
		{
			fs::path candidate = fs::path(base_path) / filename;

			std::error_code ec;
			if (fs::exists(candidate, ec) && !ec)
			{
				return normalize_path(fs::absolute(candidate).string());
			}
		}

		return {};
	}

	std::string StdFileSystem::normalize_path(const std::string& path) const
	{
		std::error_code ec;
		const auto normalized = fs::path(path).lexically_normal();
		return ec ? path : normalized.string();
	}

	void StdFileSystem::setup_default_search_path()
	{
		add_search_path((fs::path(m_base_path) / "resources/fonts").string());
	    add_search_path((fs::path(m_base_path) / "resources/textures").string());
	    add_search_path((fs::path(m_base_path) / "resources/shaders").string());
	    add_search_path((fs::path(m_base_path) / "resources/models").string());
	}
	
	std::string StdFileSystem::find_root_path()
	{
		// Finding project root (where CMakeLists.txt is)
	    const std::string& project_root = []() -> std::string
		{
	        std::vector<std::string> possible_paths = {
	            "../../../..",		// build/bin/../../../.. -> project root
	            "../../..",  		// build/bin/../../.. -> project root
	            "../..",     		// build/../.. -> project root
	            "..",        		// bin/.. -> project root
	            ".",         		// current directory
	            ""           		// executable directory
	        };
	        
	        for (const std::string& rel_path : possible_paths)
	        {
	            std::filesystem::path test_path = std::filesystem::current_path();
	            if (!rel_path.empty())
	            {
	                test_path = test_path / rel_path;
	            }
	            
	            // Checking if this looks like our project root
	            if (std::filesystem::exists(test_path / "CMakeLists.txt") ||
	                std::filesystem::exists(test_path / "resources") ||
	                std::filesystem::exists(test_path / "src"))
	            {
	                return test_path.string();
	            }
	        }
	        
	        return std::filesystem::current_path().string();
	    }();

	    return project_root;
	}

} // namespace c2l::core::filesystem