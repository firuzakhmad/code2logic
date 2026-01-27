#ifndef I_FILE_SYSTEM_HPP
#define I_FILE_SYSTEM_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <optional>

namespace c2l::core::filesystem
{
	namespace fs = std::filesystem;

	/**
	 * @brief File mode for opening files
	 */
	enum class FileMode
	{
		Read,
		Write,
		Append,
		ReadBinary,
		WriteBinary
	};

	/**
	* @brief File system statistics
	*/
	struct FileStats
	{
		uint64_t size											{};
		std::chrono::system_clock::time_point last_modified;
		bool is_directory										{};
		bool exists												{};
	};

	/** 
	* @brief File system interface for cross-platform file operations
	* 
	* This interface provides abstract file system operations that can be
	* implemented for different platforms (Windows, Linux, macOS)
	*/
	class IFileSystem
	{
	public:
		virtual ~IFileSystem() = default;

		/**
		 * @brief Check if a file or directory exists
		 * @param fs::path& The path to check 
		 * @return true if exists, false otherwise
		 */
		[[nodiscard]] virtual bool exists(
			const fs::path& path) const =  0;

		/**
		* @brief Read entire text file into string
		* @param fs::path Path to the text file
		* @return File contents as std::optional<std::string>
		*/
		[[nodiscard]] virtual std::optional<std::string> read_text(
			const fs::path& path) const = 0;

		/**
		 * @brief Read entire binary file into byte vector
		 * @param path Path to the binary file
		 * @return File contents as byte vector
		 */
		[[nodiscard]] virtual std::optional<std::vector<uint8_t>> read_binary(
			const fs::path& path) const = 0;

		/**
		 * @brief Write string content to text file
		 * @param path Path where to write file
		 * @param content String content to write
		 * @return true if successful, false otherwise
		 */
		[[nodiscard]] virtual bool write_text(
			const fs::path& path, 
			const std::string& content) const = 0;

		/**
		 * @brief Write binary data to file
		 * @param path Path where to write the file
		 * @param data binary data to write
		 * @return true if successful, false otherwise
		 */
		virtual bool write_binary(
			const fs::path& path, 
			const std::vector<uint8_t>& data) = 0;

		/**
		 * @brief Delete a file or empty directory
		 * @param path Path to delete_file
		 * @return true if successful, false otherwise
		 */
		virtual bool delete_file(const fs::path& path) = 0;

		/** 
		 * @brief Create a directory (including parent directories if needed)
		 * @param path Directory path to create
		 * @return true if successful, false otherwise
		 */
		virtual bool create_directory(const fs::path& path) = 0;

		/**
		 * @brief List contents of a directory 
		 * @param path Directory path to list
		 * @param recursive Whether to list recursively
		 * @return Vector of fs::path file names in a directory
		 */
		[[nodiscard]] virtual std::vector<fs::path> list_directory(
			const fs::path& path,
			bool recursive) const = 0;

		/**
		 * @brief Find a file by its name
		 * @param start_dir Start searching directory for the file
		 * @param filename File to search (icon.png, config.json etc.)
		 * @return std::optional<fs::path> name of the file if found, 
		   otherwise std::nullopt
		 */ 
	    [[nodiscard]] virtual std::optional<fs::path> find_file_recursive(
	    	const fs::path& start_dir,
	    	const std::string& filename) const = 0;

		/**
		 * @brief Get absolute path from relative path
		 * @param path Relative or absolute path
		 * @return std::optional<fs::path> absolude path if found, 
		   otherwise std::nullopt
		 */
		[[nodiscard]] virtual std::optional<fs::path> get_absolute_path(
			const fs::path& path) const = 0;
		
		/**
		 * @brief Get current working directory
		 * @return fs::path current working directory path.
		 */ 
		[[nodiscard]] virtual fs::path 
		get_working_directory() const = 0;

		/**
		 * @brief Get file/directory statistics
		 * @param path Path to get stats for
		 * @return FileStats structure with file information
		 */ 
		[[nodiscard]] virtual FileStats get_file_stats(
			const fs::path& path) const = 0;

		/**
		 * @brief Get file size in bytes
		 * @param path Path to the file 
		 * @return File in bytes, 0, if file doesn't exit
		 */
		[[nodiscard]] virtual std::optional<uint64_t> get_file_size(
			const fs::path& path) const = 0;

		/**
		 * @brief Get last modification time
		 * @param path Path to the file/directory
		 * @return std::nullopt if last modification time fails
		 */
		[[nodiscard]] virtual std::optional<std::chrono::system_clock::time_point>  
		get_last_modified(const fs::path& path) const = 0;

		/**
		 * @brief Get last write time
		 * @param path Path to the file/directory
		 * @return Returns std::nullopt if the file does not exist or cannot be read.
		 */
		[[nodiscard]] virtual std::optional<fs::file_time_type>
		get_last_write(const fs::path& path) const = 0;

		/**
		 * @brief Add a search path for resource resolution
		 * @param path Search path to add
		 */
		virtual void add_search_path(const fs::path& path) = 0;

		/**
		 * @brief Remove a search path
		 * @param path Search path to remove
		 */
		virtual void remove_search_path(const fs::path& path) = 0;  


		/**
		 * @brief Resolve a relative path using search paths
		 * @param relative_path Relative path to resolve
		 * @return @return std::optional<fs::path> Absolute path if found,
	       otherwise std::nullopt
		 */
		[[nodiscard]] virtual std::optional<fs::path> resolve_path(
			const fs::path& relative_path
		) const = 0;

		/**
	     * @brief Set the base path for relative path resolution
	     * @param path Base path to set
     	 */
		virtual void set_base_path(const fs::path& path) = 0; 

		/**
	     * @brief Get the base path
	     * @return std::optional<fs::path> Current base path,
	       otherwise std::nullopt
	     */
		[[nodiscard]] virtual fs::path get_base_path() const = 0;
	}; 

} // c2l::core::filesystem

#endif // I_FILE_SYSTEM_HPP