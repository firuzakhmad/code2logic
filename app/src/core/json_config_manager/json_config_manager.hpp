#ifndef CODE2LOGIC_JSON_CONFIG_MANAGER_HPP
#define CODE2LOGIC_JSON_CONFIG_MANAGER_HPP

#include "core/file_system/i_file_system.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "algorithms/algorithm_types.hpp"

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
     * @brief Configuration for JsonConfigManager
     */
    struct JsonConfigManagerConfig
    {
        std::filesystem::path config_base_path = "resources";
        bool enable_hot_reloading = false;
        std::chrono::milliseconds hot_reload_check_interval{2000};
        bool enable_validation = true;
        size_t max_cached_configs = 50;
        ThreadManager::ThreadType loader_thread_type = 
            ThreadManager::ThreadType::IO;

        explicit JsonConfigManagerConfig() = default;
    };

    /**
     * @class JsonConfigManager
     * @brief JSON files configuration manager with 
     * ThreadManager integration
     * 
     * Features:
     * - Async loading via ThreadManager's thread pool
     * - Hot reloading using dedicated thread from ThreadManager
     * - LRU caching with automatic eviction
     * - Type-safe configuration access
     * - Schema validation support
     * - Performance monitoring
     */
    class JsonConfigManager final
    {
    public:
        /**
         * @brief Callback for configuration changes
         */
        using ConfigChangedCallback = std::function<void(
            const std::string& config_name,
            const nlohmann::json& old_config,
            const nlohmann::json& new_config
        )>;

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
         * @brief Gets statistics
         */
        struct Statistics
        {
            std::atomic<size_t> total_configs_loaded     {0};
            std::atomic<size_t> total_configs_failed     {0};
            std::atomic<size_t> cache_hits              {0};
            std::atomic<size_t> cache_misses            {0};
            std::atomic<size_t> memory_usage_bytes      {0};
            std::atomic<uint64_t> total_load_time_ms    {0};
            std::atomic<size_t> hot_reloads_performed   {0};

            Statistics() = default;

            Statistics(const Statistics& other) 
            {
                total_configs_loaded = other.total_configs_loaded.load();
                total_configs_failed = other.total_configs_failed.load();
                cache_hits = other.cache_hits.load();
                cache_misses = other.cache_misses.load();
                memory_usage_bytes = other.memory_usage_bytes.load();
                total_load_time_ms = other.total_load_time_ms.load();
                hot_reloads_performed = other.hot_reloads_performed.load();
            }

            Statistics& operator=(const Statistics& other) 
            {
                if (this != &other) {
                    total_configs_loaded = other.total_configs_loaded.load();
                    total_configs_failed = other.total_configs_failed.load();
                    cache_hits = other.cache_hits.load();
                    cache_misses = other.cache_misses.load();
                    memory_usage_bytes = other.memory_usage_bytes.load();
                    total_load_time_ms = other.total_load_time_ms.load();
                    hot_reloads_performed = other.hot_reloads_performed.load();

                }
                return *this;
            }

            std::chrono::milliseconds get_total_load_time() const 
            {
                return std::chrono::milliseconds(total_load_time_ms.load());
            }
        };


        JsonConfigManager(
            filesystem::IFileSystem& filesystem,
            ThreadManager& thread_manager,
            const JsonConfigManagerConfig& config = JsonConfigManagerConfig{}
        );

        ~JsonConfigManager();

        // Non-copyable, non-movable
        JsonConfigManager(const JsonConfigManager&) = delete;
        JsonConfigManager& operator=(const JsonConfigManager&) = delete;
        JsonConfigManager(JsonConfigManager&&) = delete;
        JsonConfigManager& operator=(JsonConfigManager&&) = delete;

        /**
         * @brief Loads application configuration 
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
            const algorithms::AlgorithmType& algorithm_type,
            bool async = true
        );

        /**
         * @brief Loads a custom configuration file
         */
        std::future<LoadResult> load_config(
            const std::string& config_name,
            const std::filesystem::path& path,
            bool async = true,
            bool persistent = false
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
            const algorithms::AlgorithmType& algorithm_type) const;

        /**
         * @brief Gets algorithm time complexity
         */
        std::string get_algorithm_time_complexity(
            const algorithms::AlgorithmType& algorithm_type) const;

        /**
         * @brief Gets algorithm space complexity
         */
        std::string get_algorithm_space_complexity(
            const algorithms::AlgorithmType& algorithm_type) const;

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
            const std::string& config_name, 
            const std::string& error_message)> callback
        );


        /**
         * @brief Enables/disables hot reloading
         */
        void enable_hot_reloading(bool enable);

        /**
         * @brief Checks for configuration file changes
         * @note Usually called automatically by hot reload thread
         */
        void check_for_changes();

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
        struct ConfigEntry 
        {
            nlohmann::json data;
            std::filesystem::path file_path;
            std::filesystem::file_time_type last_write_time;
            LoadResult load_result;
            std::chrono::steady_clock::time_point last_access;
            bool persistent{false};
            bool validated{false};
            size_t memory_usage{0};
        };

        struct LoadRequest 
        {
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

        /**
         * @brief Reads and parses JSON file
         */
        nlohmann::json load_json_file_sync(const std::filesystem::path& file_path);

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
         * @brief Updates cache access time
         * @note Call with m_cache_mutex locked
         */
        void update_cache_access_locked(const std::string& config_name);

        /**
         * @brief Performs cache cleanup (LRU eviction)
         */
        void cleanup_cache();

        /**
         * @brief Starts hot reload monitoring thread via ThreadManager
         */
        void start_hot_reload_monitor();

        /**
         * @brief Stops hot reload monitoring
         */
        void stop_hot_reload_monitor();

        /**
         * @brief Hot reload loop function (executed in ThreadManager thread)
         */
        void hot_reload_loop();


        filesystem::IFileSystem& m_file_system;
        ThreadManager& m_thread_manager;
        JsonConfigManagerConfig m_config;

        mutable std::shared_mutex m_cache_mutex;
        std::unordered_map<std::string, ConfigEntry> m_config_cache;

        std::atomic<bool> m_running{true};
        std::atomic<bool> m_hot_reload_enabled{false};
        std::atomic<bool> m_hot_reload_thread_active{false};

        std::string m_hot_reload_thread_name;
        std::mutex m_hot_reload_mutex;
        std::condition_variable m_hot_reload_cv;

        // Callbacks
        ConfigChangedCallback m_config_changed_callback;
        std::function<void(const std::string&, const std::string&)> m_error_callback;

        // Statistics
        mutable Statistics m_stats;
    };


    template<typename T>
    std::optional<T> JsonConfigManager::get(
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

        const_cast<JsonConfigManager*>(this)->update_cache_access_locked(config_name);

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

        // Navigating JSON pointer
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
    T JsonConfigManager::get_or(
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

#endif //CODE2LOGIC_JSON_CONFIG_MANAGER_HPP