#include "core/resources/texture_resource.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/utils.hpp"

#include "../../../dependencies/stb_image/stb_image.h"

namespace c2l::core::resources
{
	TextureResource::TextureResource(const std::string& path,
	 								 core::filesystem::IFileSystem& file_system,
	 								 const std::vector<uint8_t>* texture_data,
	 								 const bool stbi_set_flip_vertically_on_load,
	 								 const bool upload_to_gpu)
		: m_path{path}
		, m_file_system{file_system}
		,  m_last_access_time{std::chrono::steady_clock::now()}
		,  m_stbi_set_flip_vertically_on_load{stbi_set_flip_vertically_on_load}
		, m_upload_to_gpu{upload_to_gpu}
	{
		if (texture_data != nullptr && !texture_data->empty())
		{
			m_texture_data = *texture_data;
			m_owns_texture_data = false;
			m_data_loaded = true;
		}
	}

 	TextureResource::~TextureResource() { cleanup(); }

 	bool TextureResource::load()
 	{
 		if (m_state == ResourceState::Loading)
 		{
 			LOG_WARNING("TextureResource already loading: {}", m_path);
 			return false;
 		}

		if (m_state == ResourceState::Loaded && m_gpu_uploaded && m_texture_id != 0)
		{
			update_access_time();
			return true;
		}

 		m_state = ResourceState::Loading;

 		bool success = false;

		if (!m_data_loaded)
		{
			if (!m_texture_data.empty())
			{
				success = load_from_memory();
			}
			else
			{
				success = load_from_file();
			}
		}
		else
		{
			success = true; // Data already loaded
		}

 		if (success && m_upload_to_gpu && !m_gpu_uploaded)
        {
            success = upload_to_gpu();
        }

        if (success)
        {
            m_state = ResourceState::Loaded;
            update_access_time();
        }
        else
        {
            m_state = ResourceState::Error;
            unload();
        }

 		return success;
 	}

	bool TextureResource::load_data_only()
	{
		if (m_data_loaded)
		{
			return true; // Data already loaded
		}

		if (m_state == ResourceState::Loading)
		{
			LOG_WARNING("TextureResource already loading: {}", m_path);
			return false;
		}

		m_state = ResourceState::Loading;

		bool success = false;
		if (!m_texture_data.empty())
		{
			success = load_from_memory();
		}
		else
		{
			success = load_from_file();
		}

		if (success)
		{
			m_data_loaded = true;
			m_state = ResourceState::Loaded; // Mark as loaded (data only)
			update_access_time();
		}
		else
		{
			m_state = ResourceState::Error;
			unload();
		}

		return success;
	}

	void TextureResource::unload()
	{
		if (m_texture_id != 0)
		{
			GLuint texture_id = get_gl_texture_id();
			glDeleteTextures(1, &texture_id);
			m_texture_id = 0;
			m_gpu_uploaded = false;
		}

		if (m_owns_texture_data)
		{
			std::vector<uint8_t>().swap(m_texture_data);
		}

		m_owns_texture_data = false;
		m_data_loaded = false;

		m_width = 0;
		m_height = 0;
		m_channels = 0;
		m_memory_usage = 0;
		m_state = ResourceState::Unloaded;
	}


	bool TextureResource::is_loaded() const
	{
		// Consider loaded if either data is loaded or fully loaded with GPU
		return m_state == ResourceState::Loaded && (m_data_loaded || m_gpu_uploaded);
	}

	uint64_t TextureResource::get_memory_usage() const
	{
		return m_memory_usage;
	}

	void TextureResource::generate_mipmaps() const
	{
		if (m_texture_id != 0 && m_state == ResourceState::Loaded)
		{
			GLuint texture_id = get_gl_texture_id();
			glBindTexture(GL_TEXTURE_2D, texture_id);
			glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void TextureResource::set_filtering(const int min_filter, const int max_filter) const
	{
		if (m_texture_id != 0 && m_state == ResourceState::Loaded)
		{
			GLuint texture_id = get_gl_texture_id();
			glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, max_filter);
            glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void TextureResource::set_wrapping(const int wrap_s, const int wrap_t) const
	{
        if (m_texture_id != 0 && m_state == ResourceState::Loaded)
        {
            GLuint texture_id = get_gl_texture_id();
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

	bool TextureResource::load_from_file()
	{
		try
		{
			if (!m_file_system.exists(m_path))
			{
				LOG_ERROR("TextureResource file not found: {}", m_path);
				return false;
			}

			m_path = m_file_system.resolve_path(m_path);
			m_texture_data = m_file_system.read_binary(m_path);
			m_data_loaded = true;
			return true; // Just loading data only, no GPU upload yet
		}
		catch(const std::exception& e)
		{
			LOG_ERROR("Failed to load texture from file: {} - {}", m_path, e.what());
			return false;
		}
	}

	bool TextureResource::load_from_memory()
	{
		if (m_texture_data.empty())
		{
			LOG_ERROR("TextureResource no texture data available");
			return false;
		}

		// Just mark as loaded, don't process the data yet
		m_data_loaded = true;
		return true;
	}

	bool TextureResource::upload_to_gpu()
	{
		if (!m_data_loaded)
		{
			LOG_ERROR("Cannot upload to GPU: texture data not loaded");
			return false;
		}

		if (m_gpu_uploaded)
		{
			return true; // Already uploaded
		}

		if (m_state == ResourceState::Loading)
		{
			LOG_WARNING("TextureResource already loading: {}", m_path);
			return false;
		}

		m_state = ResourceState::Loading;

		// Create a copy of the data since stbi_load_from_memory needs raw pointer
		std::vector<unsigned char> image_copy(m_texture_data.begin(), m_texture_data.end());

		stbi_set_flip_vertically_on_load(m_stbi_set_flip_vertically_on_load);

		unsigned char* image_data = stbi_load_from_memory(
			image_copy.data(),
			static_cast<int>(image_copy.size()),
			&m_width,
			&m_height,
			&m_channels,
			4 // Force 4 channels (RGBA)
		);

		if (!image_data)
		{
			LOG_ERROR("TextureResource failed to load image for GPU upload: {}", stbi_failure_reason());
			m_state = ResourceState::Error;
			return false;
		}

		bool success = upload_to_gpu_internal(image_data);

		stbi_image_free(image_data);

		if (success)
		{
			m_gpu_uploaded = true;
			m_state = ResourceState::Loaded;
			update_access_time();
		}
		else
		{
			m_state = ResourceState::Error;
		}

		return success;
	}

    bool TextureResource::upload_to_gpu_internal(const unsigned char* image_data)
    {
		if (!image_data)
		{
			LOG_ERROR("TextureResource no image data to upload");
			return false;
		}

		// Calculate memory usage
		m_memory_usage = static_cast<uint64_t>(m_width) * static_cast<uint64_t>(m_height) * 4; // 4 channels

		// Generate OpenGL texture
		GLuint texture_id;
		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		// Set default texture parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Upload texture data
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

		// Unbind texture
		glBindTexture(GL_TEXTURE_2D, 0);

		// Store texture ID
		m_texture_id = core::utils::gl_texture_to_im_texture(texture_id);

		return true;
    }


	GLuint TextureResource::get_gl_texture_id() const
	{
		if (m_texture_id != 0 && m_state == ResourceState::Loaded)
        {
        	return core::utils::im_texture_to_gl_texture(m_texture_id);
        }

        return 0;
	}
	
	std::chrono::steady_clock::time_point TextureResource::get_last_access_time() const
	{
		return m_last_access_time;
	}
	

	void TextureResource::update_access_time() 
    { 
        m_last_access_time = std::chrono::steady_clock::now(); 
    }

    void TextureResource::cleanup()
    {
		if (m_state != ResourceState::Unloaded)
		{
			unload();
		}
    }

} // namespace c2l::core::resources