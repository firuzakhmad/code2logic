#include "core/json_config_manager/json_config_manager.hpp"
#include "core/utils/timer/timer.hpp"

#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace c2l::core
{
    JsonConfigManager::JsonConfigManager(
        filesystem::IFileSystem &filesystem,
        ThreadManager &thread_manager,
        const JsonConfigManagerConfig &config)
            : m_file_system{filesystem}
            , m_thread_manager{thread_manager}
            , m_config{config}
    {
        // Ensuring config directory exists
        auto config_path = m_file_system.resolve_path(m_config.config_base_path);
        if (!config_path || !m_file_system.exists(*config_path)) 
        {
            LOG_WARNING(
                "Config directory does not exist: {}", 
                m_config.config_base_path.string()
            );
        }

        // Starting hot reload monitor if enabled
        if (m_config.enable_hot_reloading)
        {
            start_hot_reload_monitor();
        }

        LOG_INFO("JsonConfigManager initialized");
    }

    JsonConfigManager::~JsonConfigManager()
    {
        LOG_DEBUG("JsonConfigManager shutting down");

        m_running.store(false, std::memory_order_release);

        stop_hot_reload_monitor();

        unload_all();

        LOG_DEBUG("JsonConfigManager destroyed");
    }

    std::future<JsonConfigManager::LoadResult> JsonConfigManager::load_app_config(
        const std::filesystem::path &path,
        bool async)
    {
        return load_config("app_config", path, async, true);
    }

    std::future<JsonConfigManager::LoadResult> JsonConfigManager::load_algorithm_config(
        const algorithms::AlgorithmType& algorithm_type,
        bool async)
    {
        std::string algorithm_filename =
            std::string(algorithms::algorithm_id(algorithm_type)) + ".json";
        std::filesystem::path path =
            std::filesystem::path{"resources"} / "algorithms" / algorithm_filename;
        return load_config(
            std::string(algorithms::algorithm_id(algorithm_type)), 
            path, 
            async,
            true
        );
    }

    std::future<JsonConfigManager::LoadResult>
    JsonConfigManager::load_available_algorithms_config(
        const std::filesystem::path &path,
        bool async)
    {
        return load_config("available_algorithms_config", path, async, true);
    }

    std::future<JsonConfigManager::LoadResult> JsonConfigManager::load_theme_config(
        const std::string& theme_name,
        bool async)
    {
        std::filesystem::path path = std::filesystem::path("themes") / (theme_name + ".json");
        return load_config("theme_" + theme_name, path, async, true);
    }

    std::future<JsonConfigManager::LoadResult> JsonConfigManager::load_icon_config(
        const std::filesystem::path& path,
        bool async)
    {
        return load_config("icon_config", path, async, true);
    }

    std::future<JsonConfigManager::LoadResult> JsonConfigManager::load_config(
        const std::string &config_name,
        const std::filesystem::path &path,
        bool async,
        bool persistent)
    {
        auto full_path = m_file_system.resolve_path(path);
        const std::string path_str = full_path ? full_path->string() : path.string();

        if (!full_path || !m_file_system.exists(*full_path))
        {
            LOG_ERROR("Config file not found: {}", path_str);

            std::promise<LoadResult> promise;
            promise.set_value(LoadResult{
                false,
                "File not found: " + path_str,
                std::chrono::steady_clock::now(),
                0
            });

            return promise.get_future();
        }

        // Checking if already loaded
        {
            std::unique_lock unique_lock(m_cache_mutex);

            auto it = m_config_cache.find(config_name);
            if (it != m_config_cache.end())
            {
                std::promise<LoadResult> promise;
                LoadResult result = it->second.load_result;
                promise.set_value(it->second.load_result);
                const_cast<JsonConfigManager*>(this)->update_cache_access_locked(
                    config_name
                );
                ++m_stats.cache_hits;
                return promise.get_future();
            }
        }

        ++m_stats.cache_misses;

        if (async)
        {
            // Creating promise for async result
            auto request = std::make_shared<LoadRequest>();
            request->config_name = config_name;
            request->file_path = *full_path;
            request->validate = m_config.enable_validation;
            request->persistent = persistent;

            std::future<LoadResult> future = request->promise.get_future();

            // Using ThreadManager for async loading 
            m_thread_manager.enqueue_task(
                m_config.loader_thread_type,
                [this, request]() mutable
                {
                    try
                    {
                        LoadResult result = load_config_sync(
                            request->config_name,
                            request->file_path,
                            request->validate
                        );
                        request->promise.set_value(result);
                    }
                    catch(const std::exception& e)
                    {
                        LOG_ERROR(
                            "Async load failed for {}: {}",
                            request->config_name, 
                            e.what()
                        );

                        request->promise.set_value(
                            LoadResult
                            {
                                false,
                                e.what(),
                                std::chrono::steady_clock::now(),
                                0
                            }
                        );
                    }
                    
                }
            );

            LOG_DEBUG("Async load queued via ThreadManager: {}", config_name);
            return future;
        } 
        else 
        {
            // Synchronous load
            LoadResult result = load_config_sync(
                config_name,
                *full_path,
                m_config.enable_validation
            );

            std::promise<LoadResult> promise;
            promise.set_value(result);
            return promise.get_future();
        }
    }

    std::future<std::vector<JsonConfigManager::LoadResult>> 
    JsonConfigManager::load_config_directory(
        const std::filesystem::path &directory,
        bool recursive,
        bool async)
    {
        auto full_path = m_file_system.resolve_path(directory);
        if (!full_path)
        {
            LOG_ERROR("Failed to resolve directory: ", directory.string());
        }

        auto load_func = [this, full_path, recursive]() -> std::vector<LoadResult>
        {
            std::vector<LoadResult> results;

            try
            {
                auto files = m_file_system.list_directory(*full_path, recursive);

                for (const auto& file : files)
                {
                    if (file.extension() == ".json")
                    {
                        std::string config_name = file.stem().string();
                        auto result = load_config_sync(
                            config_name,
                            file
                        );

                        results.push_back(result);

                        if (!result.success)
                        {
                            LOG_ERROR("Failed to load config {}: {}",
                                        file.string(), result.error_message);
                        }
                    }
                }
            } catch (const std::exception& e) 
            {
                LOG_ERROR(
                    "Failed to load config directory {}: {}",
                    full_path->string(), 
                    e.what()
                );
            }

            return results;
        };

        if (async)
        {
            return m_thread_manager.enqueue_task(
                m_config.loader_thread_type,
                std::move(load_func)
            );
        }
        else
        {
            std::promise<std::vector<LoadResult>> promise;
            promise.set_value(load_func());
            return promise.get_future();
        }
    }

    std::string JsonConfigManager::get_string(
        const std::string& config_name,
        const std::string& json_pointer,
        const std::string& default_value) const
    {
        auto value = get<std::string>(config_name, json_pointer);
        return value.has_value() ? *value : default_value;
    }

    int JsonConfigManager::get_int(
        const std::string& config_name,
        const std::string& json_pointer,
        int default_value) const
    {
        auto value = get<int>(config_name, json_pointer);
        return value ? *value : default_value;
    }

    float JsonConfigManager::get_float(
        const std::string& config_name,
        const std::string& json_pointer,
        float default_value) const
    {
        auto value = get<float>(config_name, json_pointer);
        return value ? *value : default_value;
    }

    bool JsonConfigManager::get_bool(
        const std::string& config_name,
        const std::string& json_pointer,
        bool default_value) const
    {
        auto value = get<bool>(config_name, json_pointer);
        return value ? *value : default_value;
    }

    std::vector<std::string> JsonConfigManager::get_string_array(
        const std::string& config_name,
        const std::string& json_pointer,
        const std::vector<std::string>& default_value) const
    {
        auto value = get<std::vector<std::string>>(config_name, json_pointer);
        return value ? *value : default_value;
    }


    nlohmann::json JsonConfigManager::get_algorithm_config(
        const algorithms::AlgorithmType& algorithm_type) const
    {
        return get<nlohmann::json>(
            std::string(algorithms::algorithm_id(algorithm_type)))
                    .value_or(nlohmann::json{});
    }

    nlohmann::json JsonConfigManager::get_available_algorithms_config() const
    {
        return get<nlohmann::json>("available_algorithms_config")
            .value_or(nlohmann::json{});
    }

    nlohmann::json JsonConfigManager::get_theme_config(
        const std::string& theme_name) const 
    {
        return get<nlohmann::json>("theme_" + theme_name).value_or(nlohmann::json{});
    }

    std::string JsonConfigManager::get_theme_color(
        const std::string& theme_name,
        const std::string& color_key,
        const std::string& default_color) const
    {
        // TODO: 
        return {};
    }

    nlohmann::json JsonConfigManager::get_icon_config() const
    {
        return get<nlohmann::json>("icon_config").value_or(nlohmann::json{});
    }

    std::optional<std::filesystem::path> JsonConfigManager::get_icon_path(
        const std::string& icon_name,
        const std::string& pack) const
    {
        auto config = get_icon_config();

        try
        {
            if (!config.contains("icons") || !config["icons"].is_object())
            {
                return std::nullopt;
            }

            const auto& icons = config["icons"];
            if (!icons.contains(icon_name))
            {
                return std::nullopt;
            }

            const auto& icon_entry = icons[icon_name];

            if (!pack.empty() && icon_entry.contains("alternatives") && 
                icon_entry["alternatives"].is_object())
            {
                const auto& alternatives = icon_entry["alternatives"];
                if (alternatives.contains(pack) && alternatives[pack].is_string())
                {
                    return std::filesystem::path(alternatives[pack].get<std::string>());
                }
            }

            if (icon_entry.contains("default_path") && icon_entry["default_path"].is_string())
            {
                return std::filesystem::path(icon_entry["default_path"].get<std::string>());
            }

        } catch (...) {
            return std::nullopt;
        }
 
        return std::filesystem::path{
            "resources/icons/default/" + 
            icon_name + "/" + icon_name + ".png"
        };
    }


    std::unordered_map<std::string, std::string> JsonConfigManager::get_ui_icons(
        const std::string& ui_component) const
    {
        // TODO: 
        return {};
    }

    bool JsonConfigManager::is_loaded(const std::string& config_name) const
    {
        std::shared_lock lock(m_cache_mutex);
        return m_config_cache.find(config_name) != m_config_cache.end();
    }

    JsonConfigManager::LoadResult JsonConfigManager::get_load_status(
        const std::string& config_name) const
    {
        std::shared_lock lock(m_cache_mutex);

        auto it = m_config_cache.find(config_name);
        if (it != m_config_cache.end())
        {
            return it->second.load_result;
        }

        return LoadResult {
            false,
            "Config not loaded",
            std::chrono::steady_clock::now(),
            0
        };
    }

    std::future<JsonConfigManager::LoadResult> JsonConfigManager::reload_config(
        const std::string& config_name,
        bool async)
    {
        std::shared_lock lock(m_cache_mutex);

        auto it = m_config_cache.find(config_name);
        if (it == m_config_cache.end())
        {
            LOG_WARNING("Cannot reload unloaded config: {}", config_name);

            std::promise<LoadResult> promise;
            promise.set_value(LoadResult{
                false,
                "Config not loaded: " + config_name,
                std::chrono::steady_clock::now(),
                0
            });

            return promise.get_future();
        }

        bool persistent = it->second.persistent;
        const std::filesystem::path file_path = it->second.file_path;
        lock.unlock();

        return load_config(config_name, file_path, async, persistent);
    }

    bool JsonConfigManager::unload_config(
        const std::string& config_name)
    {
        std::unique_lock lock(m_cache_mutex);

        auto it = m_config_cache.find(config_name);
        if (it == m_config_cache.end() || it->second.persistent) 
        {
            return false;
        }

        m_stats.memory_usage_bytes -= it->second.memory_usage;
        m_config_cache.erase(it);

        LOG_DEBUG("Unloaded config: {}", config_name);
        return true;
    }

    void JsonConfigManager::unload_all()
    {
        std::unique_lock lock(m_cache_mutex);

        size_t unloaded = 0;
        size_t memory_freed = 0;

        for (auto it = m_config_cache.begin(); it != m_config_cache.end();)
        {
            if (!it->second.persistent)
            {
                memory_freed += it->second.memory_usage;
                it = m_config_cache.erase(it);
                unloaded++;
            } else 
            {
                ++it;
            }
        }

        m_stats.memory_usage_bytes -= memory_freed;

        LOG_DEBUG(
            "Unloaded {} configs, freed {} bytes", 
            unloaded, 
            memory_freed
        );
    }

    void JsonConfigManager::set_config_changed_callback(
        ConfigChangedCallback callback)
    {
        m_config_changed_callback = std::move(callback);
    }

    void JsonConfigManager::set_error_callback(
        std::function<void(const std::string&, const std::string&)> callback)
    {
        m_error_callback = std::move(callback);
    }

    void JsonConfigManager::enable_hot_reloading(bool enable)
    {
        if (enable == m_hot_reload_enabled.load())
        {
            return;
        }

        m_hot_reload_enabled.store(
            enable, 
            std::memory_order_release
        );

        if (enable)
        {
            start_hot_reload_monitor();
        } else {
            stop_hot_reload_monitor();
        }
    }

    void JsonConfigManager::check_for_changes()
    {
        std::vector<std::pair<std::string, std::filesystem::file_time_type>> updates;      
        {
            {
                std::shared_lock lock(m_cache_mutex);

                for (auto& [config_name, entry] : m_config_cache)
                {
                    try
                    {
                        auto current_time = m_file_system.get_last_write(entry.file_path);

                        if (current_time && *current_time > entry.last_write_time)
                        {
                            updates.emplace_back(config_name, *current_time);
                        }
                    } catch (const std::exception& e) 
                    {
                        LOG_WARNING(
                            "Failed to check file time for {}: {}",
                            config_name, 
                            e.what()
                        );
                    }
                }
            }

            if (updates.empty()) 
            {
                return;
            }

            // Updating timestamps under write lock
            {
                std::unique_lock lock(m_cache_mutex);
                for (auto& [name, time] : updates)
                {
                    auto it = m_config_cache.find(name);
                    if (it != m_config_cache.end())
                    {
                        it->second.last_write_time = time;
                    }
                }
            }
            
        }

        // Reload changed configs
        for (const auto& [name, time] : updates)
        {
            LOG_INFO("Config changed, reloading: {}", name);
            reload_config(name, true);
            ++m_stats.hot_reloads_performed;
        }
    }

    JsonConfigManager::Statistics JsonConfigManager::get_statistics() const
    {
        Statistics result;

        result.total_configs_loaded = m_stats.total_configs_loaded.load();
        result.total_configs_failed = m_stats.total_configs_failed.load();
        result.cache_hits          = m_stats.cache_hits.load();
        result.cache_misses        = m_stats.cache_misses.load();
        result.memory_usage_bytes  = m_stats.memory_usage_bytes.load();
        result.total_load_time_ms  = m_stats.total_load_time_ms.load();
        result.hot_reloads_performed = m_stats.hot_reloads_performed.load();

        return result;
    }

    std::string JsonConfigManager::dump_to_string() const
    {
        std::shared_lock lock(m_cache_mutex);

        std::ostringstream ss;
        ss << "JsonConfigManager Cache (" << m_config_cache.size() << " configs):\n";
        ss << "======================================\n";

        for (const auto& [name, entry] : m_config_cache) {
            ss << "[" << name << "]\n";
            ss << "  Path: " << entry.file_path << "\n";
            ss << "  Size: " << entry.memory_usage << " bytes\n";
            ss << "  Validated: " << (entry.validated ? "yes" : "no") << "\n";
            ss << "  Persistent: " << (entry.persistent ? "yes" : "no") << "\n";

            if (!entry.load_result.success) {
                ss << "  ERROR: " << entry.load_result.error_message << "\n";
            }

            ss << "\n";
        }

        return ss.str();
    }

    bool JsonConfigManager::validate_config(const std::string& config_name) const
    {
        std::shared_lock lock(m_cache_mutex);

        auto it = m_config_cache.find(config_name);
        if (it == m_config_cache.end())
        {
            return false;
        }

        return it->second.validated;
    }

    JsonConfigManager::LoadResult JsonConfigManager::load_config_sync(
        const std::string& config_name,
        const std::filesystem::path& file_path,
        bool validate)
    {
        auto start_time = std::chrono::steady_clock::now();

        try
        {
            LOG_DEBUG(
                "Loading config synchronously: {} from {}", 
                config_name, 
                file_path.string()
            );

            nlohmann::json config = load_json_file_sync(file_path);

            // Validating if requested
            bool is_valid = true;
            std::string validation_error;

            if (validate)
            {
                // Determining config type from name or content
                std::string config_type;
                if (config_name.find("algorithm_") == 0)
                {
                    config_type = "algorithm";
                    is_valid = validate_algorithm_config(config);
                } 
                else if (config_name.find("theme_") == 0)
                {
                    config_type = "theme";
                    is_valid = validate_theme_config(config);
                } 
                else if (config_name == "icon_config")
                {
                    config_type = "icon";
                    is_valid = validate_icon_config(config);
                } 
                else 
                {
                    is_valid = validate_json_schema(config, "generic");
                }

                if (!is_valid)
                {
                    validation_error = "Failed validation for " + config_type + " config";
                }
            }

            // Calculating memory usage
            size_t memory_usage = config.dump().size();

            // Creating load result
            LoadResult result;
            result.success = is_valid;
            result.error_message = validation_error;
            result.load_time = std::chrono::steady_clock::now();
            result.file_size = static_cast<size_t>(
                std::filesystem::file_size(file_path)
            );


            // Preparing in cache
            ConfigEntry entry;
            entry.data = std::move(config);
            entry.file_path = file_path;
            try
            {
                entry.last_write_time = *m_file_system.get_last_write(file_path);
            } 
            catch (...) 
            {
                entry.last_write_time = std::filesystem::file_time_type::min();
            }
            entry.validated = is_valid;
            entry.memory_usage = memory_usage;
            entry.load_result = result;
            entry.last_access = start_time;

            // Storing old data for callback (if this is an update)
            nlohmann::json old_data;
            bool was_updated = false;

            // Updating cache
            {
                std::unique_lock lock(m_cache_mutex);

                // Checking if this is an update to an existing config
                auto it = m_config_cache.find(config_name);
                if (it != m_config_cache.end()) 
                {
                    was_updated = true;
                    old_data = it->second.data; // Copying old data for callback
                    m_stats.memory_usage_bytes -= it->second.memory_usage;
                    it->second = std::move(entry);
                } else 
                {
                    // New config
                    m_config_cache[config_name] = std::move(entry);
                }
                
                m_stats.memory_usage_bytes += memory_usage;
            }

            // Update statistics
            auto load_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time
            );

            if (is_valid) 
            {
                ++m_stats.total_configs_failed;
            } else 
            {
                ++m_stats.total_configs_failed;
            }
            m_stats.total_load_time_ms += load_duration.count();

            // Trigger change callback if this was an update
            // Note: Callback called outside lock for safety
            if (was_updated && m_config_changed_callback && is_valid) 
            {
                try 
                {
                    // Only triggering callback if data actually changed
                    if (old_data != entry.data) 
                    {
                        m_config_changed_callback(config_name, old_data, entry.data);
                    }
                } catch (const std::exception& e) 
                {
                    LOG_ERROR("Config change callback failed for {}: {}", config_name, e.what());
                }
            }

            LOG_INFO("Loaded config: {} ({} bytes, validated: {}, took {}ms)",
                    config_name, result.file_size, is_valid, load_duration.count());

            // Cleanuping cache if needed
            cleanup_cache();

            return result;

        } catch (const std::exception& e) 
        {
            LOG_ERROR("Failed to load config {}: {}", config_name, e.what());

            if (m_error_callback) 
            {
                try 
                {
                    m_error_callback(config_name, e.what());
                } catch (...) {}
            }

            ++m_stats.total_configs_failed;

            return LoadResult{
                false,
                e.what(),
                std::chrono::steady_clock::now(),
                0
            };
        }
    }

    nlohmann::json JsonConfigManager::load_json_file_sync(
        const std::filesystem::path& file_path)
    {
        // Reading file content
        auto content = m_file_system.read_binary(file_path);

        if (!content || content->empty())
        {
            throw std::runtime_error("Empty or non-existent file: " + file_path.string());
        }

        try
        {
            return nlohmann::json::parse(*content);
        } catch (const nlohmann::json::parse_error& e) 
        {
            throw std::runtime_error(
                "JSON parse error at byte " +
                std::to_string(e.byte) + ": " + e.what()
            );
        }
    }

    // Validation
    bool JsonConfigManager::validate_json_schema(
        const nlohmann::json& config,
        const std::string& config_type
    ) const
    {
        // Basic JSON validation
        if (!config.is_object() && !config.is_array()) 
        {
            LOG_WARNING("Config {} is not an object or array", config_type);
            return false;
        }

        // TODO: Implement proper JSON schema validation
        // For now, just check it's valid JSON
        return true;
    }

    bool JsonConfigManager::validate_algorithm_config(const nlohmann::json& config) const
    {
        // TODO: 
        return true;
    }

    bool JsonConfigManager::validate_theme_config(
        const nlohmann::json& config
    ) const
    {
        // TODO: 
        return true;
    }

    bool JsonConfigManager::validate_icon_config(
        const nlohmann::json& config
    ) const
    {
        if (!config.contains("icons") || !config["icons"].is_object())
        {
            LOG_WARNING("Icon config missing 'icons' object");
            return false;
        }

        return true;
    }


    std::filesystem::path JsonConfigManager::resolve_config_path(
        const std::filesystem::path& relative_path) const
    {
        std::filesystem::path base = m_config.config_base_path;
        return base / relative_path;
    }

    void JsonConfigManager::update_cache_access_locked
    (const std::string& config_name)
    {
        auto it = m_config_cache.find(config_name);
        if (it != m_config_cache.end()) 
        {
            it->second.last_access = std::chrono::steady_clock::now();
        }
    }

    void JsonConfigManager::cleanup_cache()
    {
        std::unique_lock lock(m_cache_mutex);

        if (m_config_cache.size() <= m_config.max_cached_configs)
        {
            return;
        }

        std::vector<std::pair<std::string, 
                    std::chrono::steady_clock::time_point>> access_times;

        for (const auto& [name, entry] : m_config_cache)
        {
            if (!entry.persistent)
            {
                access_times.emplace_back(name, entry.last_access);
            }
        }

        // Sorting by last access (oldest first)
        std::sort(
            access_times.begin(), 
            access_times.end(),
            [](const auto& a, const auto& b) 
            {
                return a.second < b.second;
            }
        );

        // Remove oldest configs until we're under the limit
        size_t to_remove = m_config_cache.size() - m_config.max_cached_configs;
        to_remove = std::min(to_remove, access_times.size());

        for (size_t i = 0; i < to_remove; ++i) 
        {
            const auto& [name, _] = access_times[i];

            m_stats.memory_usage_bytes -= m_config_cache[name].memory_usage;

            m_config_cache.erase(name);
            LOG_DEBUG("Evicted config from cache: {}", name);
        }
    }

    // Hot Reloading
    void JsonConfigManager::start_hot_reload_monitor() 
    {
        if (m_hot_reload_thread_active.load()) 
        {
            return; // Already running
        }

        m_hot_reload_enabled.store(true, std::memory_order_release);
        m_hot_reload_thread_active.store(true, std::memory_order_release);

        // Generating unique thread name
        static std::atomic<uint64_t> hot_reload_counter{0};
        m_hot_reload_thread_name = "HotReload-" + std::to_string(++hot_reload_counter);

        // Starting dedicated thread via ThreadManager
        m_thread_manager.start_dedicated_thread(
            ThreadManager::ThreadType::BACKGROUND,
            m_hot_reload_thread_name,
            [this]() { hot_reload_loop(); },
            false // Don't auto-restart
        );

        LOG_DEBUG("Hot reload monitor started via ThreadManager");
    }

    void JsonConfigManager::stop_hot_reload_monitor() 
    {
        m_hot_reload_enabled.store(false, std::memory_order_release);

        if (m_hot_reload_thread_active.load()) {
            // Notifying thread to stop
            {
                std::unique_lock lock(m_hot_reload_mutex);
                m_hot_reload_cv.notify_all();
            }

            // Stopping the dedicated thread via ThreadManager
            if (!m_hot_reload_thread_name.empty()) {
                m_thread_manager.stop_dedicated_thread(
                    m_hot_reload_thread_name,
                    std::chrono::seconds(2)
                );
            }

            m_hot_reload_thread_active.store(false, std::memory_order_release);
        }

        LOG_DEBUG("Hot reload monitor stopped");
    }

    void JsonConfigManager::hot_reload_loop() 
    {
        LOG_DEBUG("Hot reload thread started");

        while (m_hot_reload_enabled.load(std::memory_order_acquire) &&
               m_running.load(std::memory_order_acquire)) 
               {

            // Waiting for check interval or shutdown
            {
                std::unique_lock lock(m_hot_reload_mutex);
                m_hot_reload_cv.wait_for(
                    lock,
                    m_config.hot_reload_check_interval,
                    [this]() {
                        return !m_hot_reload_enabled.load(std::memory_order_acquire) ||
                               !m_running.load(std::memory_order_acquire);
                    }
                );
            }

            if (!m_hot_reload_enabled.load(std::memory_order_acquire) ||
                !m_running.load(std::memory_order_acquire)) {
                break;
            }

            // Checking for file changes
            check_for_changes();
        }

        LOG_DEBUG("Hot reload thread exiting");
    }

 } // namespace c2l::core