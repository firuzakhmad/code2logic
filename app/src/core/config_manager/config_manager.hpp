#ifndef CODE2LOGIC_CONFIG_MANAGER_HPP
#define CODE2LOGIC_CONFIG_MANAGER_HPP

#include "core/file_system/i_file_system.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"

#include <nlohmann/json.hpp>
#include <filesystem>
#include <cstddef>
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <future>
#include <queue>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <imgui.h>

namespace c2l::core
{
    /**
     * @brief Configuration for ConfigManager
     */
    struct ConfigManagerConfig
    {
        std::filesystem::path config_base_path = "resources";
        bool enable_hot_reloading = false;
        std::chrono::milliseconds hot_reload_check_interval{2000};
        bool enable_async_loading = true;
        bool enable_validation = true;
        size_t max_cached_configs = 50;

        explicit ConfigManagerConfig() = default;
    };

    class ConfigManager
    {
    public:
        /**
         * @brief Result of a configuration operation
         */
        struct LoadResult
        {
            bool success{false};
            std::string error_message;
            std::chrono::steady_clock::time_point load_time;
            size_t file_size{0};

            explicit operator bool() const noexcept { return success; }
        };

        /**
         * @brief Callback for configuration changes
         */
        using ConfigChangedCallback = std::function<void(
            const std::string& config_name,
            const nlohmann::json& old_config,
            const nlohmann::json& new_config
        )>;

        ConfigManager(
            filesystem::IFileSystem& filesystem,
            ThreadManager& thread_manager,
            const ConfigManagerConfig& config = ConfigManagerConfig{}
        );

        ~ConfigManager();

        // Non-copyable, non-movable
        ConfigManager(const ConfigManager&) = delete;
        ConfigManager& operator=(const ConfigManager&) = delete;
        ConfigManager(ConfigManager&&) = delete;
        ConfigManager& operator=(ConfigManager&&) = delete;

        /**
         * @brief Loads application configuration (synchronous or async)
         */
        std::future<LoadResult> load_app_config(
            const std::filesystem::path& path = 
                std::filesystem::path{"resources"} / "configs" / "app_config.json",
            bool async = true
        );

        /**
         * @brief Loads theme configuration
         */
        std::future<LoadResult> load_theme_config(
            const std::string& theme_name = "dark",
            bool async = true
        );

        /**
         * @brief Loads icon configuration
         */
        std::future<LoadResult> load_icon_config(
            const std::filesystem::path& path =
                std::filesystem::path{"resources"} / "configs" / "icon_config.json",
            bool async = true
        );

        /**
         * @brief Loads algorithm configuration
         */
        std::future<LoadResult> load_algorithm_config(
            const std::string& algorithm_id,
            bool async = true
        );

        /**
         * @brief Loads a custom configuration file
         */
        std::future<LoadResult> load_config(
            const std::string& config_name,
            const std::filesystem::path& path,
            bool async = true
        );

        /**
         * @brief Loads all configuration in a directory
         */
        std::future<std::vector<LoadResult>> load_config_directory(
            const std::filesystem::path& directory,
            bool recursive = false,
            bool async = true
        );

        /**
         * @brief Gets a configuration value with type safety
         */
        template<typename T = nlohmann::json>
        std::optional<T> get(
            const std::string& config_name,
            const std::string& json_pointer = ""
        ) const;

        template<typename T>
        T get_or(
            const std::string& config_name,
            const std::string& json_pointer,
            const T& default_value = T{}
        ) const;

        /**
         * @brief Gets a string value
         */
        std::string get_string(
            const std::string& config_name,
            const std::string& json_pointer = "",
            const std::string& default_value = ""
        ) const;

        /**
         * @brief Gets an integer value
         */
        int get_int (
            const std::string& config_name,
            const std::string& json_pointer = "",
            int default_value = 0
        ) const;

        /**
         * @brief Gets a float value
         */
        float get_float (
            const std::string& config_name,
            const std::string& json_pointer = "",
            float default_value = 0.0f
        ) const;

        /**
         * @brief Gets a boolean value
         */
        bool get_bool (
            const std::string& config_name,
            const std::string& json_pointer = "",
            bool default_value = false
        ) const;

        /**
         * @brief Gets a vector of strings
         */
        std::vector<std::string> get_string_array(
            const std::string& config_name,
            const std::string& json_pointer = "",
            const std::vector<std::string>& default_value = {}
        ) const;


        /**
         * @brief Gets complete algorithm configuration
         */
        nlohmann::json get_algorithm_config(
            const std::string& algorithm_id) const;

        /**
         * @brief Gets algorithm description
         */
        std::string get_algorithm_description(
            const std::string& algorithm_id) const;

        /**
         * @brief Gets algorithm time complexity
         */
        std::string get_algorithm_time_complexity(
            const std::string& algorithm_id) const;

        /**
         * @brief Gets algorithm space complexity
         */
        std::string get_algorithm_space_complexity(
            const std::string& algorithm_id) const;

        /**
         * @brief Gets algorithm variables metadata
         */
        std::vector<nlohmann::json> get_algorithm_variables(
            const std::string& algorithm_id) const;

        /**
         * @brief Gets algorithm properties
         */
        std::unordered_map<std::string, std::string> get_algorithm_properties(
            const std::string& algorithm_id) const;

        /**
         * @brief Gets algorithm step types
         */
        std::vector<nlohmann::json> get_algorithm_step_types(
            const std::string& algorithm_id) const;


        // Theme specific Helpers
        /**
         * @brief Gets theme configuration
         */
        nlohmann::json get_theme_config(const std::string& theme_name) const;

        /**
         * @brief Gets theme color as hex string
         */
        std::string get_theme_color(
            const std::string& theme_name,
            const std::string& color_key,
            const std::string& default_color = "#FFFFFF"
        ) const;

        // Icon-specific Helpers
        /**
         * @brief Gets icon configuration
         */
        nlohmann::json get_icon_config() const;

        /**
         * @brief Gets icon path
         */
        std::optional<std::filesystem::path> get_icon_path(
            const std::string& icon_name,
            const std::string& pack = "default"
        ) const;

        /**
         * @brief Gets all icons for a UI component
         */
        std::unordered_map<std::string, std::string> get_ui_icons(
            const std::string& ui_component
        ) const;


        // Configuration Management
        /**
         * @brief Checks if a configuration is loaded
         */
        bool is_loaded(const std::string& config_name) const;

        /**
         * @brief Gets load status of a configuration
         */
        LoadResult get_load_status(const std::string& config_name) const;

        /**
         * @brief Reloads a configuration
         */
        std::future<LoadResult> reload_config(
            const std::string& config_name,
            bool async = true
        );


        /**
         * @brief Unloads a configuration
         */
        bool unload_config(const std::string& config_name);

        /**
         * @brief Unloads all configurations except persistent ones
         */
        void unload_all();

        // Callbacks and Events
        /**
         * @brief Sets callback for configuration changes
         */
        void set_config_changed_callback(ConfigChangedCallback callback);

        /**
         * @brief Sets callback for configuration loading errors
         */
        void set_error_callback(std::function<void(
            const std::string&, 
            const std::string&)> callback
        );


        // Hot Reloading
        /**
         * @brief Enables/disables hot reloading
         */
        void enable_hot_reloading(bool enable);

        /**
         * @brief Checks for configuration file changes
         */
        void check_for_changes();


        // Statistics and Debug
        /**
         * @brief Gets statistics
         */
        struct Statistics
        {
            std::atomic<size_t> total_config_loaded     {0};
            std::atomic<size_t> total_config_failed     {0};
            std::atomic<size_t> cache_hits              {0};
            std::atomic<size_t> cache_misses            {0};
            std::atomic<size_t> memory_usage_bytes      {0};
            std::atomic<uint64_t> total_load_time_ms    {0};

            Statistics() = default;

            Statistics(const Statistics& other) 
            {
                total_config_loaded = other.total_config_loaded.load();
                total_config_failed = other.total_config_failed.load();
                cache_hits = other.cache_hits.load();
                cache_misses = other.cache_misses.load();
                memory_usage_bytes = other.memory_usage_bytes.load();
                total_load_time_ms = other.total_load_time_ms.load();
            }

            Statistics& operator=(const Statistics& other) 
            {
                if (this != &other) {
                    total_config_loaded = other.total_config_loaded.load();
                    total_config_failed = other.total_config_failed.load();
                    cache_hits = other.cache_hits.load();
                    cache_misses = other.cache_misses.load();
                    memory_usage_bytes = other.memory_usage_bytes.load();
                    total_load_time_ms = other.total_load_time_ms.load();
                }
                return *this;
            }

            std::chrono::milliseconds get_total_load_time() const 
            {
                return std::chrono::milliseconds(total_load_time_ms.load());
            }
        };

        Statistics get_statistics() const;

        /**
         * @brief Dumps all configuration to string (debug)
         */
        std::string dump_to_string() const;

        /**
         * @brief Validates a configuration against schema
         */
        bool validate_config(const std::string& config_name) const;

    private:
        struct ConfigEntry {
            nlohmann::json data;
            std::filesystem::path file_path;
            std::filesystem::file_time_type last_write_time;
            LoadResult load_result;
            std::chrono::steady_clock::time_point last_access;
            bool persistent{false};
            bool validated{false};
            size_t memory_usage{0};
        };

        struct LoadRequest {
            std::string config_name;
            std::filesystem::path file_path;
            std::promise<LoadResult> promise;
            bool validate{true};
            bool persistent{false};
        };


        // File loading
        LoadResult load_config_sync(
            const std::string& config_name,
            const std::filesystem::path& path,
            bool validate = true
        );

        nlohmann::json load_json_file_sync(const std::filesystem::path& file_path);
        std::future<LoadResult> load_config_async(const LoadRequest& request);

        // Validation
        bool validate_json_schema(
            const nlohmann::json& config,
            const std::string& config_type
        ) const;

        bool validate_algorithm_config(const nlohmann::json& config) const;
        bool validate_theme_config(const nlohmann::json& config) const;
        bool validate_icon_config(const nlohmann::json& config) const;

        
        std::filesystem::path resolve_config_path(
            const std::filesystem::path& relative_path
        ) const;

        std::optional<std::reference_wrapper<const nlohmann::json>> find_json_pointer(
            const nlohmann::json& config,
            const std::string& json_pointer
        ) const;

        /**
         * @brief Updates the last access of the loaded file in the cache
         * 
         * @note Non-threadsafe
         */ 
        void update_cache_access(const std::string& config_name);
        void cleanup_cache();

        void start_hot_reload_monitor();
        void stop_hot_reload_monitor();
        void hot_reload_loop();

        void loader_thread_loop();


        filesystem::IFileSystem& m_file_system;
        ThreadManager& m_thread_manager;
        ConfigManagerConfig m_config;

        mutable std::shared_mutex m_cache_mutex;
        std::unordered_map<std::string, ConfigEntry> m_config_cache;

        std::atomic<bool> m_running{true};
        std::atomic<bool> m_hot_reload_enabled{false};

        std::thread m_hot_reload_thread;
        std::condition_variable m_hot_reload_cv;
        std::mutex m_hot_reload_mutex;

        // Callbacks
        ConfigChangedCallback m_config_changed_callback;
        std::function<void(const std::string&, const std::string&)> m_error_callback;

        // Statistics
        mutable std::mutex m_stats_mutex;
        mutable Statistics m_stats;

        // Load request queue for async loading
        std::queue<LoadRequest> m_load_queue;
        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cv;
        std::thread m_loader_thread;
    };


    template<typename T>
    std::optional<T> ConfigManager::get(
        const std::string& config_name,
        const std::string& json_pointer
    ) const 
    {
        std::shared_lock lock(m_cache_mutex);

        const auto it = m_config_cache.find(config_name);
        if (it == m_config_cache.end()) {
            ++m_stats.cache_misses;
            return std::nullopt;
        }

        ++m_stats.cache_hits;

        // If json_pointer is empty, return entire config
        if (json_pointer.empty()) 
        {
            try 
            {
                return it->second.data.get<T>();
            } catch (...) {
                return std::nullopt;
            }
        }

        // Navigate JSON pointer
        try 
        {
            nlohmann::json::json_pointer ptr(json_pointer);
            const nlohmann::json& value = it->second.data.at(ptr);
            return value.get<T>();
        } catch (...) {
            return std::nullopt;
        }
    }

    template<typename T>
    T ConfigManager::get_or(
    const std::string& config_name,
    const std::string& json_pointer,
    const T& default_value
    ) const 
    {
        if (auto v = get<T>(config_name, json_pointer)) {
            return *v;
        }
        return default_value;
    }

} // namespace c2l::core

#endif //CODE2LOGIC_CONFIG_MANAGER_HPP