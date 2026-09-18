#include "std_file_system.hpp"
#include "core/utils/logger/logger.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <climits>
#endif

namespace c2l::core::filesystem
{
	
	StdFileSystem::StdFileSystem()
	{
		const auto root_path = find_root_path();
		if (root_path)
			m_base_path = *root_path;
		else 
			LOG_ERROR("Failed to find root path");

		setup_default_search_path();
	}

	StdFileSystem::StdFileSystem(const std::string& base_path)
	{
		set_base_path(base_path);

		setup_default_search_path();
	}

	bool StdFileSystem::exists(const fs::path& path) const
	{
		return resolve_path(path).has_value();
	}

	std::optional<std::string> StdFileSystem::read_text(const fs::path& path) const
	{
		const auto resolved_path = resolve_path(path);
		if (!resolved_path)
			return std::nullopt;

		std::error_code ec;
		const auto size = fs::file_size(*resolved_path, ec);
		if (ec)
			return std::nullopt;

		std::ifstream file(*resolved_path);
		if (!file)
			return std::nullopt;

		std::string content;
    	content.resize(static_cast<size_t>(size));

		if (!file.read(content.data(), static_cast<std::streamsize>(content.size())))
        	return std::nullopt;
		
			return content;
	}

	std::optional<std::vector<uint8_t>> StdFileSystem::read_binary(const fs::path& path) const
	{
		const auto resolved_path = resolve_path(path);
		if (!resolved_path)
			return std::nullopt;

		std::ifstream file(*resolved_path, std::ios::binary | std::ios::ate);
		if (!file)
			return std::nullopt;

		const std::streamsize size = file.tellg();
		if (size < 0)
			return std::nullopt;

		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
			return std::nullopt;

		return buffer;
	}

	bool StdFileSystem::write_text(
		const fs::path& path, 
		const std::string& content) const
	{
		const fs::path dir = path.parent_path();
		if (!dir.empty() && !fs::exists(dir))
		{
			std::error_code ec;
			fs::create_directories(dir, ec);
			if (ec)
    			return false;
		}

		std::ofstream file(path);
		if (!file)
        	return false;

		file.write(content.data(), static_cast<std::streamsize>(content.size()));
		return file.good();
	}

	bool StdFileSystem::write_binary(
		const fs::path& path, 
		const std::vector<uint8_t>& data)
	{
		const fs::path dir = path.parent_path();
		if (!dir.empty() && !fs::exists(dir))
		{
			std::error_code ec;
			fs::create_directories(dir, ec);
			if (ec)
    			return false;
		}

		std::ofstream file(path, std::ios::binary);
		if (!file.is_open()) 
			return false;

		std::streamsize size = static_cast<std::streamsize>(data.size());
		file.write(reinterpret_cast<const char*>(data.data()), size);
		
		return file.good();
	}

	bool StdFileSystem::delete_file(const fs::path& path)
	{
		std::error_code ec;
		fs::remove(path, ec);
		return !ec;
	}

	bool StdFileSystem::create_directory(const fs::path& path)
	{
		std::error_code ec;
		fs::create_directories(path, ec);
		return !ec;
	}

	std::vector<fs::path> StdFileSystem::list_directory(
		const fs::path& path, bool recursive) const
	{
		std::vector<fs::path> result;

		std::error_code ec;

		if (recursive)
		{
			for (fs::recursive_directory_iterator it(path, ec), end;
				 it != end && !ec; 
				 it.increment(ec))
			{
				result.emplace_back(it->path());
			}
		} 
		else 
		{
			for (fs::directory_iterator it(path, ec), end;
				 it != end && !ec; 
				 it.increment(ec))
			{
				result.emplace_back(it->path());
			}
		}

		return result;
	}

	std::optional<fs::path> StdFileSystem::get_absolute_path(
		const fs::path& path) const 
	{
		std::error_code ec;
		fs::path result = fs::absolute(path, ec);

		if (ec)
			return std::nullopt;

		return result;
	}

	fs::path StdFileSystem::get_working_directory() const 
	{
	    return fs::current_path();
	}

	core::filesystem::FileStats StdFileSystem::get_file_stats(
		const fs::path& path) const 
	{
		core::filesystem::FileStats stats{};

		std::error_code ec;

		const auto status = fs::status(path, ec);
		if (ec)
			return stats;

		stats.exists = fs::exists(status);
		stats.is_directory = fs::is_directory(status);

		if (!stats.is_directory)
		{
			stats.size = fs::file_size(path, ec);
			if (ec)
				stats.size = 0;
		}

		const auto ftime = fs::last_write_time(path, ec);
		if (!ec)
		{
			stats.last_modified =
				std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ftime - fs::file_time_type::clock::now()
				+ std::chrono::system_clock::now());
		}

		return stats;
	}


	std::optional<uint64_t> StdFileSystem::get_file_size(
		const fs::path& path) const
	{
		std::error_code ec;
		const auto size = fs::file_size(path, ec);
		if (ec)
			return std::nullopt;

		return size;
	}

	std::optional<std::chrono::system_clock::time_point> 
	StdFileSystem::get_last_modified(
		const fs::path& path) const
	{
		std::error_code ec;
		const auto ftime = fs::last_write_time(path, ec);
		if (ec)
			return std::nullopt;

		return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			ftime - fs::file_time_type::clock::now()
			+ std::chrono::system_clock::now());
	}

	std::optional<fs::file_time_type> 
	StdFileSystem::get_last_write(const fs::path& path) const
	{
		std::error_code ec;
		auto ftime = fs::last_write_time(path, ec);

		if (ec)
			return std::nullopt;

		return ftime;
	}

	void StdFileSystem::add_search_path(const fs::path& path)
	{
		auto abs = get_absolute_path(path);
		if (!abs)
			return;

		auto normalized = normalize_path(*abs);
		if (!normalized)
			return;

		m_search_paths.insert(*normalized);
	}

	void StdFileSystem::remove_search_path(const fs::path& path)
	{
		auto abs = get_absolute_path(path);
		if (!abs)
			return;

		m_search_paths.erase(*abs);
	}

	std::optional<fs::path> StdFileSystem::resolve_path(
		const fs::path& relative_path) const 
	{
		std::error_code ec;

		if (relative_path.is_absolute())
			return fs::exists(relative_path, ec) && !ec
				? std::optional(relative_path)
				: std::nullopt;

		fs::path base = m_base_path / relative_path;
		if (fs::exists(base, ec) && !ec)
			return base;

		for (const auto& search_path : m_search_paths)
		{
			fs::path candidate = search_path / relative_path;
			if (fs::exists(candidate, ec) && !ec)
				return candidate;
		}

		return std::nullopt;
	}

	void StdFileSystem::set_base_path(const fs::path& path)
	{
		auto abs = get_absolute_path(path);
		if (!abs)
			throw std::runtime_error("Invalid base path");
		
		m_base_path = *abs;
	}

	fs::path StdFileSystem::get_base_path() const 
	{
		return m_base_path;
	}

    std::optional<fs::path> StdFileSystem::find_file_recursive(
    	const fs::path& start_dir, 
    	const std::string& filename) const
    {
    	std::error_code ec;
		for (fs::recursive_directory_iterator it(start_dir, ec), end;
			it != end && !ec;
			it.increment(ec))
		{
			if (it->path().filename() == filename)
				return it->path();
		}

		return std::nullopt;
    }


	std::optional<fs::path> StdFileSystem::find_in_search_paths(
		const std::string& filename) const
	{
		if (filename.empty())
			return std::nullopt;

		for (const auto& base_path : m_search_paths)
		{
			fs::path candidate = base_path / filename;

			std::error_code ec;
			if (!fs::exists(candidate, ec) || ec)
				continue;

			auto abs = get_absolute_path(candidate);
			if (!abs)
				continue;

			return normalize_path(*abs);
		}

		return std::nullopt;
	}

	std::optional<fs::path> StdFileSystem::normalize_path(
		const fs::path& path) const
	{
		return path.lexically_normal();
	}

	void StdFileSystem::setup_default_search_path()
	{
		add_search_path(m_base_path / "resources" / "fonts");
	    add_search_path(m_base_path / "resources" / "textures");
	    add_search_path(m_base_path / "resources" / "icons");
	    add_search_path(m_base_path / "resources" / "shaders");
	    add_search_path(m_base_path / "resources" / "models");
	}

	namespace
    {
		std::optional<fs::path> get_executable_path()
    	{
#if defined(__APPLE__)
        	char buf[PATH_MAX];
	        uint32_t size = sizeof(buf);

	        if (_NSGetExecutablePath(buf, &size) != 0)
	            return std::nullopt;

	        std::error_code ec;
	        auto resolved = fs::canonical(buf, ec);
	        return ec ? std::nullopt : std::optional(resolved);
#elif defined(_WIN32)
	        wchar_t buf[MAX_PATH];
	        DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);

	        if (len == 0 || len == MAX_PATH)
	            return std::nullopt;

	        return fs::path(buf);
#elif defined(__linux__)
	        char buf[PATH_MAX];
	        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);

	        if (len == -1)
	            return std::nullopt;

	        buf[len] = '\0';
	        return fs::path(buf);
#else
        	return std::nullopt;
#endif
        }
    }
	 
	std::optional<fs::path> StdFileSystem::find_root_path()
    {
        if (auto exe_path = get_executable_path())
        {
#if defined(__APPLE__)
            // Inside a .app bundle, bundled resources live in
            // Contents/Resources, a sibling of Contents/MacOS/.
            std::string exe_str = exe_path->string();
            auto macos_pos = exe_str.find("/Contents/MacOS/");
            if (macos_pos != std::string::npos)
            {
                fs::path resources_dir =
                    fs::path(exe_str.substr(0, macos_pos)) / "Contents" / "Resources";
                if (fs::exists(resources_dir))
                    return resources_dir;
            }
#endif
            // Windows/Linux: resources are copied next to the executable.
            fs::path exe_dir = exe_path->parent_path();
            if (fs::exists(exe_dir / "resources"))
                return exe_dir;
        }

        // Development fallback
        std::error_code ec;
        fs::path current = fs::current_path(ec);
        if (ec)
            return std::nullopt;

        while (!current.empty())
        {
            if (fs::exists(current / "CMakeLists.txt") ||
                fs::exists(current / "resources") ||
                fs::exists(current / "src"))
            {
                return current;
            }

            fs::path parent = current.parent_path();
            if (parent == current)
                break;

            current = parent;
        }

        return std::nullopt;
    }

} // namespace c2l::core::filesystem