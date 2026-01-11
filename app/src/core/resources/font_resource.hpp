#ifndef FONT_RESOURCE_HPP
#define FONT_RESOURCE_HPP

#include "core/resources/i_resource.hpp"
#include "core/file_system/i_file_system.hpp"

#include <vector>

struct ImFont;

namespace c2l::core::resources
{
	/**
	 * @brief Font resource for loading and managing TTF/OTF fonts
	 * 
	 * Supports ImGui integration and font scaling.
	 */
	class FontResource final : public IResource
	{
	public:
		/**
		 * @brief Construct a font resource
		 * @param path Path to the font file
		 * @param file_system File system for loading
		 * @param size_pixels Font size in pixels
		 * @param font_data Optional preloaded font data
		 */
		FontResource(std::string  path,
					  core::filesystem::IFileSystem& file_system,
					  float size_pixels = 16.0f,
					  const std::vector<uint8_t>* font_data = nullptr);

		~FontResource() override;

		// IResource implementation
		/**
		 * @copydoc IResource::get_path
		 */
		[[nodiscard]] const std::string& get_path() const override;

		/**
		 * @copydoc IResource::get_state
		 */
		[[nodiscard]] ResourceState get_state() const override;

		/**
		 * @copydoc IResource::is_loaded
		 */
		[[nodiscard]] bool is_loaded() const override;

		/**
		 * @copydoc IResource::load
		 */
	    bool load() override;

		/**
		 * @copydoc IResource::unload
		 */
		void unload() override;

		/**
		 *
		 * @copydoc IResource::get_memory_usage
		 */
		[[nodiscard]] uint64_t get_memory_usage() const override;

		/**
		 * @copydoc IResource::get_last_access_time
		 */
		[[nodiscard]] std::chrono::steady_clock::time_point get_last_access_time() const override;

		/**
		 * @copydoc IResource::update_access_time
		 */
	    void update_access_time() override;


	    [[nodiscard]] float get_size_pixels() const;

		void set_size_pixels(float size);

	    /**
	     * @brief Get ImFont pointer for use with ImGui
	     * @return ImFont pointer, nullptr if not loaded
	     */
	    [[nodiscard]] ImFont* get_im_font() const;

	    /**
	     * @brief Get raw font data
	     * @return Reference to font data vector
	     */
	    [[nodiscard]] const std::vector<uint8_t>& get_font_data() const;

	private:
    	std::string m_path;
    	ResourceState m_state{ResourceState::Unloaded};
    	filesystem::IFileSystem& m_file_system;
    	std::chrono::steady_clock::time_point m_last_access_time;

    	std::vector<uint8_t> m_font_data;
	    float m_size_pixels;
	    ImFont* m_font      	{nullptr};
	    bool m_owns_font_data	{true};
	    
	    bool load_from_file();
	    bool load_from_memory();

		/**
		 * @note Requires a valid ImGui context on the calling thread.
		 *
		 * @return true if font loaded, false otherwise
		 */
		bool build_font();
	    void cleanup();
	};
}

#endif // FONT_RESOURCE_HPP