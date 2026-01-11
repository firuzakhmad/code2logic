#ifndef STD_FILE_SYSTEM
#define STD_FILE_SYSTEM

#include "core/file_system/i_file_system.hpp"

#include <unordered_set>

namespace c2l::core::filesystem
{
	/**
	 * @brief Standard C++17 file system implementation 
	 * 
	 * Uses std::filesystem for cross-platform file operations.
	 * Supports Windows, Linux, and macOS.
	 */
	class StdFileSystem final : public IFileSystem
	{
	public:
		/**
	     * @brief Construct with default base path (current directory)
	     */
	    StdFileSystem();

		/**
		 * @brief Construct with specific path
		 * @param base_path path for relative path resolution
		 */ 
	    explicit StdFileSystem(const std::string& base_path);

	    ~StdFileSystem() override = default;

        // IFileSystem implementation
		/**
		 * @copydoc IFileSystem::exists
		 */
	    [[nodiscard]] bool exists(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::read_text
		 */
		[[nodiscard]] std::string read_text(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::read_binary
		 */
	    [[nodiscard]] std::vector<uint8_t> read_binary(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::write_text
		 */
	    [[nodiscard]] bool write_text(
	    	const std::string& path,
	    	const std::string& content) const override;

		/**
		 * @copydoc IFileSystem::write_binary
		 */
	    [[nodiscard]] bool write_binary(
	    	const std::string& path,
	    	const std::vector<uint8_t>& data) override;

		/**
		 * @copydoc IFileSystem::delete_file
		 */
	    [[nodiscard]] bool delete_file(const std::string& path) override;

		/**
		 * @copydoc IFileSystem::create_directory
		 */
	    bool create_directory(const std::string& path) override;

		/**
		 * @copydoc IFileSystem::list_directory
		 */
	    [[nodiscard]] std::vector<std::string> list_directory(
	    	const std::string& path,
	    	bool recursive) const override;

		/**
		 * @copydoc IFileSystem::find_file_recursive
		 */
	    [[nodiscard]] std::string find_file_recursive(
	    	const std::string& start_dir,
	    	const std::string& filename) const override;

		/**
		 * @copydoc IFileSystem::get_absolute_path
		 */
	    [[nodiscard]] std::string get_absolute_path(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::get_working_directory
		 */
		[[nodiscard]] std::string get_working_directory() const override;

		/**
		 * @copydoc IFileSystem::get_file_stats
		 */
	    [[nodiscard]] core::filesystem::FileStats get_file_stats(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::get_file_size
		 */
	    [[nodiscard]] uint64_t get_file_size(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::get_last_modified
		 */
	    [[nodiscard]] std::chrono::system_clock::time_point get_last_modified(const std::string& path) const override;

		/**
		 * @copydoc IFileSystem::get_base_path
		 */
	    [[nodiscard]] std::string get_base_path() const override;

		/**
		 * @copydoc IFileSystem::resolve_path
		 */
	    [[nodiscard]] std::string resolve_path(const std::string& relativePath) const override;

		/**
		 * @copydoc IFileSystem::add_search_path
		 */
		void add_search_path(const std::string& path) override;

		/**
		 * @copydoc IFileSystem::remove_search_path
		 */
	    void remove_search_path(const std::string& path) override;

		/**
		 * @copydoc IFileSystem::set_base_path
		 */
	    void set_base_path(const std::string& path) override;

 	private:
 		std::unordered_set<std::string> m_search_paths;
 		std::string m_base_path;

		/**
		 * @brief Searches for a file within the registered search paths.
		 *
		 * Iterates over all configured search directories and checks whether
		 * the given filename exists relative to any of them.
		 *
		 * @param filename Relative file name to search for (e.g. "Roboto.ttf").
		 *
		 * @return Absolute, normalized path to the file if found;
		 *         empty string otherwise.
		 *
		 * @note The function does not throw and performs filesystem existence checks.
		 */
 		[[nodiscard]] std::string find_in_search_paths(const std::string& filename) const;

		/**
		 * @brief Normalizes a filesystem path lexically.
		 *
		 * Converts the given path into a normalized form by resolving
		 * redundant separators, "." and ".." components without
		 * accessing the filesystem.
		 *
		 * @param path Input path string (relative or absolute).
		 *
		 * @return Normalized path string. If normalization fails,
		 *         the original path is returned unchanged.
		 *
		 * @note This function performs purely lexical normalization
		 *       and does not verify filesystem existence.
		 * @thread_safety Thread-safe.
		 */
 		[[nodiscard]] std::string normalize_path(const std::string& path) const;

		/**
		 * @brief Registers the default resource search paths.
		 *
		 * Adds standard engine resource directories (fonts, textures,
		 * shaders, models) relative to the configured base path.
		 *
		 * This function is intended to be called during filesystem
		 * initialization and modifies the internal search path list.
		 *
		 * @note Existing search paths are preserved.
		 * @thread_safety Not thread-safe. Must be called during initialization.
		 */
 		void setup_default_search_path();

		/**
		 * @brief Attempts to locate the project root directory.
		 *
		 * Heuristically determines the root directory of the project by
		 * searching parent directories for known project markers such as
		 * "CMakeLists.txt", "resources", or "src".
		 *
		 * If no project root can be identified, the current working
		 * directory is returned as a fallback.
		 *
		 * @return Absolute path to the inferred project root directory.
		 *
		 * @note This function does not guarantee correctness in all
		 *       deployment scenarios and is intended for development
		 *       and tooling use.
		 * @thread_safety Thread-safe.
		 */
 		std::string find_root_path();
	};

} // namespace c2l::core::filesystem

#endif // STD_FILE_SYSTEM