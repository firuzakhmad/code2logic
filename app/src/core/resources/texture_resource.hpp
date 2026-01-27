#ifndef CORE_RESOURCES_TEXTURE_RESOURCE_HPP
#define CORE_RESOURCES_TEXTURE_RESOURCE_HPP

#include "core/resources/i_resource.hpp"
#include "core/file_system/i_file_system.hpp"

#include <glad/glad.h>
#include <imgui.h>

namespace c2l::core::resources
{
	/**
	 * @brief Texture resource for loading and managing image textures
	 *
	 * Supports various image formats via stb_image and ImGui integration.
	 */
	 class TextureResource final : public IResource
	 {
	 public:
	 	/**
	 	 * @brief Construct a texture resource
	 	 * @param path Path to the texture file
	 	 * @param file_system File system for loading
	 	 * @param texture_data Optional preloading texture data
	 	 * @param stbi_set_flip_vertically_on_load false on default
	 	 */
	 	TextureResource(const std::filesystem::path& path,
	 					core::filesystem::IFileSystem& file_system,
	 					const std::vector<uint8_t>* texture_data = nullptr,
	 					const bool stbi_set_flip_vertically_on_load = false,
	 					const bool upload_to_gpu = false);
	 	~TextureResource() override;

	 	// IResource implementation
	 	/**
		 * @copydoc IResource::get_state()
		 */
	 	[[nodiscard]] ResourceState get_state() const override { return m_state; }

	 	/**
		 * @copydoc IResource::is_loaded()
		 */
	 	[[nodiscard]] bool is_loaded() const override;

	 	/**
		 * @copydoc IResource::get_memory_usage()
		 */
		[[nodiscard]] uint64_t get_memory_usage() const override;

	 	/**
		 * @copydoc IResource::get_last_access_time()
		 */
		[[nodiscard]] std::chrono::steady_clock::time_point get_last_access_time() const override;

	 	/**
		 * @copydoc IResource::load()
		 */
	 	bool load() override;

	 	/**
		 * @copydoc IResource::unload()
		 */
	 	void unload() override;

	 	/**
		 * @copydoc IResource::update_access_time()
		 */
	 	void update_access_time() override;

		// Texture specific interface
		[[nodiscard]] int get_width() const { return m_width; }
		[[nodiscard]] int get_height() const { return m_height; }
		[[nodiscard]] int get_channels() const { return m_channels; }

		/**
         * @brief Get ImTextureID for use with ImGui
         * @return ImTextureID, nullptr if not loaded
         */
        [[nodiscard]] ImTextureID get_texture_id() const { return m_texture_id; }

        /**
         * @brief Get GLuint for use with OpenGL
         * @return GLuint, nullptr if not loaded
         */
        [[nodiscard]] GLuint get_gl_texture_id() const;


        /**
         * @brief Get raw texture data
         * @return Reference to texture data vector
         */
        [[nodiscard]] const std::vector<uint8_t>& get_texture_data() const { return m_texture_data; }

        /**
         * @brief Generate mipmaps for the texture
         */
        void generate_mipmaps() const;

        /**
         * @brief Set texture filtering parameters
         * @param min_filter Minification filter
         * @param mag_filter Magnification filter
         */
        void set_filtering(int min_filter, int mag_filter) const;

        /**
         * @brief Set texture wrapping parameters
         * @param wrap_s Wrap mode for S coordinate
         * @param wrap_t Wrap mode for T coordinate
         */
        void set_wrapping(int wrap_s, int wrap_t) const;

	 	/**
		 * @brief Load texture data from file without GPU upload
		 * @return true if successful, false otherwise
		 */
	 	bool load_data_only();

	 	/**
		  * @brief Upload already loaded texture (memory) data to GPU
		  * @return true if successful, false otherwise
		  */
	 	bool upload_to_gpu();

	private:
	 	bool load_from_file();
	 	bool load_from_memory();
	 	bool upload_to_gpu_internal(const unsigned char*);
	 	void cleanup();

	    core::filesystem::IFileSystem& m_file_system;

	    ResourceState m_state{ResourceState::Unloaded};
	    std::chrono::steady_clock::time_point m_last_access_time;

	 	std::vector<uint8_t> m_texture_data;
	 	ImTextureID m_texture_id				{0};
	 	int m_width								{0};
	 	int m_height							{0};
	 	int m_channels							{0};
	 	uint64_t m_memory_usage					{0};
	 	bool m_owns_texture_data				{true};
	 	bool m_stbi_set_flip_vertically_on_load {false};
	 	bool m_upload_to_gpu					{false};
	 	bool m_gpu_uploaded						{false};
	 	bool m_data_loaded                      {false};
	 };

} // namespace c2l::core::resources

#endif // CORE_RESOURCES_TEXTURE_RESOURCE_HPP