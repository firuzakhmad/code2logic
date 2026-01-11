#ifndef I_FILE_SYSTEM_HPP
#define I_FILE_SYSTEM_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace c2l::core::filesystem
{
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
		 * @param path The path to check 
		 * @return true if exists, false otherwise
		 */
		[[nodiscard]] virtual bool exists(const std::string& path) const =  0;

		/**
		* @brief Read entire text file into string
		* @param path Path to the text file
		* @return File contents as string
		* @throws std::runtime_error if file cannot be read
		*/
		[[nodiscard]] virtual std::string read_text(const std::string& path) const = 0;

		/**
		 * @brief Read entire binary file into byte vector
		 * @param path Path to the binary file
		 * @return File contents as byte vector
		 * @throws std::runtime_error if file cannot be read
		 */
		[[nodiscard]] virtual std::vector<uint8_t> read_binary(const std::string& path) const = 0;

		/**
		 * @brief Write string content to text file
		 * @param path Path where to write file
		 * @param content String content to write
		 * @return true if successful, false otherwise
		 */
		[[nodiscard]] virtual bool write_text(const std::string& path, const std::string& content) const = 0;

		/**
		 * @brief Write binary data to file
		 * @param path Path where to write the file
		 * @param data binary data to write
		 * @return true if successful, false otherwise
		 */
		virtual bool write_binary(const std::string& path, const std::vector<uint8_t>& data) = 0;

		/**
		 * @brief Delete a file or empty directory
		 * @param path Path to delete_file
		 * @return true if successful, false otherwise
		 */
		virtual bool delete_file(const std::string& path) = 0;

		/** 
		 * @brief Create a directory (including parent directories if needed)
		 * @param path Directory path to create
		 * @return true if successful, false otherwise
		 */
		virtual bool create_directory(const std::string& path) = 0;

		/**
		 * @brief List contents of a directory 
		 * @param path Directory path to list
		 * @param recursive Whether to list recursively
		 * @return Vector of file names in a directory
		 */
		[[nodiscard]] virtual std::vector<std::string> list_directory(
			const std::string& path,
			bool recursive) const = 0;

		/**
		 * @brief Find a file by its name
		 * @param start_dir Start searching directory for the file
		 * @param filename File to search
		 * @return String name of the file if found
		 */ 
	    [[nodiscard]] virtual std::string find_file_recursive(const std::string& start_dir,
	    										const std::string& filename) const = 0;

		/**
		 * @brief Get absolute path from relative path
		 * @param path Relative or absolute path
		 * @return Absolute path
		 */
		[[nodiscard]] virtual std::string get_absolute_path(const std::string& path) const = 0;
		
		/**
		 * @brief Get current working directory
		 * @return current working directory path
		 */ 
		[[nodiscard]] virtual std::string get_working_directory() const = 0;

		/**
		 * @brief Get file/directory statistics
		 * @param path Path to get stats for
		 * @return FileStats structure with file information
		 */ 
		[[nodiscard]] virtual FileStats get_file_stats(const std::string& path) const = 0;

		/**
		 * @brief Get file size in bytes
		 * @param path Path to the file 
		 * @return File in bytes, 0, if file doesn't exit
		 */
		[[nodiscard]] virtual uint64_t get_file_size(const std::string& path) const = 0;

		/**
		 * @brief Get last modification time
		 * @param path Path to the file/directory
		 * @return Last modification time
		 */
		[[nodiscard]] virtual std::chrono::system_clock::time_point get_last_modified(const std::string& path) const = 0;

		/**
		 * @brief Add a search path for resource resolution
		 * @param path Search path to add
		 */
		virtual void add_search_path(const std::string& path) = 0;

		/**
		 * @brief Remove a search path
		 * @param path Search path to remove
		 */
		virtual void remove_search_path(const std::string& path) = 0;  


		/**
		 * @brief Resolve a relative path using search paths
		 * @param relative_path Relative path to resolve
		 * @return Absolute path if found, empty string if not found
		 */
		[[nodiscard]] virtual std::string resolve_path(const std::string& relative_path) const = 0;

		/**
	     * @brief Set the base path for relative path resolution
	     * @param path Base path to set
     	 */
		virtual void set_base_path(const std::string& path) = 0; 

		/**
	     * @brief Get the base path
	     * @return Current base path
	     */
		[[nodiscard]] virtual std::string get_base_path() const = 0;
	}; 

} // c2l::core::filesystem

#endif // I_FILE_SYSTEM_HPP