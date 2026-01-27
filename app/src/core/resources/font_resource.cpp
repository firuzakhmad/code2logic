#include "core/resources/font_resource.hpp"
#include "core/utils/logger/logger.hpp"

#include <chrono>
#include <utility>

#include <imgui.h>

namespace c2l::core::resources
{
	FontResource::FontResource(const std::filesystem::path& path,
							   core::filesystem::IFileSystem& file_system,
							   const float size_pixels,
							   const std::vector<uint8_t>* font_data)
	: IResource(path)
	, m_file_system{file_system}
	, m_last_access_time(std::chrono::steady_clock::now())
	, m_size_pixels{size_pixels}
	{

		if (font_data != nullptr && !font_data->empty()) 
        {
            m_font_data = *font_data;
            m_owns_font_data = false;
        }
	}

	FontResource::~FontResource() { cleanup(); }

	bool FontResource::load()
	{
        if (m_state == ResourceState::Loading)
        {
            return false;
        }

        if (m_state == ResourceState::Loaded && m_font != nullptr)
        {
            update_access_time();
            return true;
        }

        m_state = ResourceState::Loading;

        bool success = false;
        
        if (!m_font_data.empty()) 
        {
            success = load_from_memory();
        }
        else 
        {
            success = load_from_file();
        }

        if (success) 
        {
            m_state = ResourceState::Loaded;
            update_access_time();
        }
        else 
        {
            m_state = ResourceState::Error;
            LOG_ERROR("FontResource failed to load: {}", m_path.string());
            unload();
        }

		return success;
	}

	void FontResource::unload()
	{
		if (m_font != nullptr) 
	    {
	        m_font = nullptr;
	    }

	    if (m_owns_font_data) 
	    {
	        std::vector<uint8_t>().swap(m_font_data); // Clear and free memory
	    }

	    m_owns_font_data = false;


	    m_state = ResourceState::Unloaded;
	}

	uint64_t FontResource::get_memory_usage() const 
	{
		return m_font_data.size();
	}

	void FontResource::set_size_pixels(float size)
	{
        if (size <= 0.0f) 
        {
            LOG_WARNING("FontResource invalid size: {}", size);
            return;
        }

        if (std::abs(m_size_pixels - size) < 0.001f) 
        {
            return;
        }

        m_size_pixels = size;

        // If font is already loaded, rebuilding it with new size
        if (m_state == ResourceState::Loaded && !m_font_data.empty())
        {
            build_font();
        }
	}

	bool FontResource::load_from_file()
	{
		try
		{
            if (!m_file_system.exists(m_path))
            {
                LOG_ERROR("FontResource file not found: {}", m_path.string());
                return false;
            }
			const auto font_data = m_file_system.read_binary(m_path);
			if (!font_data)
			{
				LOG_ERROR("Failed to read binary font file: {}", m_path.string());
			}

			m_font_data = *font_data;

			return load_from_memory();
		} catch(const std::exception& e)
		{
			LOG_ERROR("Failed to load font {}: {}", m_path.string(), e.what());
		}
		return false;
	}

	bool FontResource::load_from_memory()
	{
        if (m_font_data.empty())
        {
            LOG_ERROR("FontResource no font data available");
            return false;
        }

        return build_font();
	}

	bool FontResource::build_font() 
	{
	    if (m_font_data.empty()) 
        {
            LOG_ERROR("FontResource no data to build font");
            return false;
        }

        if (m_size_pixels <= 0.0f) 
        {
            LOG_WARNING("FontResource invalid font size: {}", m_size_pixels);
            m_size_pixels = 16.0f; // Default fallback
        }
	    
	    // Adding font to ImGui
	    ImFontConfig font_config;
	    font_config.FontDataOwnedByAtlas = false; // We manage the memory

	    m_font = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
	        m_font_data.data(), 
	        static_cast<int>(m_font_data.size()),
	        m_size_pixels,
	        &font_config
	    );

		return m_font != nullptr;
	}

	void FontResource::cleanup()
	{
		if (m_state != ResourceState::Unloaded)
		{
			unload();
		}
	}

	ResourceState FontResource::get_state() const
	{
		return m_state;
	}

	bool FontResource::is_loaded() const
	{
		return m_state == ResourceState::Loaded;
	}

	std::chrono::steady_clock::time_point FontResource::get_last_access_time() const
	{
		return m_last_access_time;
	}

	void FontResource::update_access_time()
	{
		m_last_access_time = std::chrono::steady_clock::now();
	}

	// Font-specific interface
	float FontResource::get_size_pixels() const
	{
		return m_size_pixels;
	}

	ImFont* FontResource::get_im_font() const
	{
		return m_font;
	}

	const std::vector<uint8_t>& FontResource::get_font_data() const
	{
		return m_font_data;
	}
} // namespace c2l::core::resources
