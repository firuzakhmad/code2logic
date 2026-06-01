#ifndef CODE2LOGIC_ICON_MANAGER_HPP
#define CODE2LOGIC_ICON_MANAGER_HPP

#include "core/utils/thread_manager/thread_manager.hpp"
#include "core/resources/texture_resource.hpp"
#include "core/resources/resource_manager.hpp"
#include "core/utils/variables.hpp"
#include "core/json_config_manager/json_config_manager.hpp"

#include <string>
#include <imgui.h>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <shared_mutex>
#include <queue>
#include <atomic>
#include <future>
#include <condition_variable>
#include <unordered_set>
#include <utility>
#include <nlohmann/json.hpp>


#define DEFAULT_BUTTON_ICON_SIZE ImVec2(25, 18)


namespace c2l::ui::managers
{
    enum class IconType : uint32_t
    {
        PLAY,
        ARROW_RIGHT,
        ARROW_LEFT,
        PAUSE,
        STEP_FORWARD,
        STEP_BACKWARD,
        RESET,
        SETTINGS,
        EXPAND,
        COLLAPSE,
        CLOSE,
        INFO,
        WARNING,
        ERROR,
        SUCCESS,
        MENU,
        GRID,
        LIST,
        SEARCH,
        FILTER,
        DOWNLOAD,
        UPLOAD,
        SAVE,
        TRASH,
        EDIT,
        COPY,
        PASTE,
        UNDO,
        REDO,
        ARRAY,
        ALGORITHM,
        STATISTIC,
        STEPS,
        ALGORITHM_VISUALIZATION,
        ALGORITHM_COMPARISON,
        EXIT,
        CHECK,
        LINK,

        UNKNOWN,
        COUNT = 64
    };

    enum class IconState : uint8_t
    {
        UNLOADED,
        LOADING_IO,
        LOADING_GPU,
        READY,
        FAILED,
        UNLOADING
    };

    enum class IconQuality : uint8_t
    {
        LOW,
        MEDIUM,
        HIGH,
        ULTRA
    };

    struct IconConfig
    {
        std::string path;
        ImVec2 uv0          {0, 0};
        ImVec2 uv1          {1, 1};
        IconQuality quality {IconQuality::MEDIUM};
        bool preload        {true};
        bool persistent     {false};
        bool flip_vertical  {true};
        int max_retries     {3};
        c2l::core::ThreadManager::ThreadType preferred_thread_type
        {
            c2l::core::ThreadManager::ThreadType::IO
        };

        IconConfig() = default;

        IconConfig(
            std::string path,
            const IconQuality quality = IconQuality::MEDIUM,
            const bool preload = true)
            : path(std::move(path))
            , quality(quality)
            , preload(preload)
        {}
    };

    struct IconData
    {
        ImTextureID texture_id      {0};
        GLuint gl_texture_id        {0};
        std::shared_ptr<c2l::core::resources::TextureResource> resource;
        ImVec2 size                 {0, 0};
        IconType type               {IconType::UNKNOWN};
        IconState state             {IconState::UNLOADED};
        std::atomic<float> progress {0.0f};
        std::chrono::steady_clock::time_point last_access;
        std::chrono::steady_clock::time_point load_time;
        size_t memory_usage         {0};
        int retry_count             {0};
        std::string error_message;

        [[nodiscard]] bool is_ready() const noexcept
        {
            return state == IconState::READY &&
                    texture_id != 0 &&
                    gl_texture_id != 0 &&
                    resource &&
                    resource->is_loaded();
        }

        [[nodiscard]] bool is_loading() const noexcept
        {
            return state == IconState::LOADING_IO ||
                   state == IconState::LOADING_GPU;
        }

        [[nodiscard]] float aspect_ratio() const noexcept
        {
            return size.y > 0 ? size.x / size.y : 1.0f;
        }
    };

    class IconManager final
    {
    public:
        using IconCallback = std::function<void(IconType)>;

        IconManager(
            c2l::core::ThreadManager& thread_manager,
            c2l::core::resources::ResourceManager& resource_manager,
            c2l::core::JsonConfigManager& json_config_manager);

        ~IconManager();

        IconManager(const IconManager&) = delete;
        IconManager& operator=(const IconManager&) = delete;

        // Registration
        bool register_icon(const IconType& type, const IconConfig& config);
        bool register_icons(const std::vector<std::pair<IconType, IconConfig>>& icons);
        bool unregister_icon(const IconType& type);

        // Loading
        void load_icon(const IconType& type, bool async = true);
        void load_icons(const std::vector<IconType>& icons);
        void preload_all();

        void unload_icon(const IconType& type);
        void unload_all(bool keep_persistent = false);
        void reload_icon(const IconType& type);
        void cleanup_cache();

        void set_memory_limit(size_t megabytes);

        void set_loaded_callback(IconCallback callback);
        void set_error_callback(IconCallback callback);

        // Rendering
        bool render_icon(
            const IconType& type,
            const ImVec2& size = DEFAULT_ICON_SIZE);
        bool render_icon_button(
            const char* str_id,
            const IconType& type,
            const ImVec2& size,
            const ImVec4& tint = {-1, 0, 0, 0},
            const char* tooltip = nullptr);

        bool render_icon_button(
            const std::string &id,
            IconType type,
            const std::function<void()> &callback,
            const ImVec2 &size,
            bool enabled,
            const std::string &tooltip
        );

        bool render_icon_text_button(
            const char* str_id,
            IconType type,
            const char* label,
            float font_scale,
            const ImVec2& size,
            const ImVec4& tint,
            const char* tooltip = nullptr
        );

        void render_loading_indicator(const ImVec2& size = {});
        void render_error_icon(const ImVec2& size = {}, const char* text = "!");

        // Getters
        [[nodiscard]] bool is_ready(const IconType& type) const;
        [[nodiscard]] bool is_loading(const IconType& type) const;
        [[nodiscard]] IconState get_state(const IconType& type) const;
        [[nodiscard]] float get_progress(const IconType& type) const;
        [[nodiscard]] ImTextureID get_texture_id(const IconType& type) const;
        [[nodiscard]] ImVec2 get_size(const IconType& type) const;
        [[nodiscard]] size_t get_loaded_count() const;
        [[nodiscard]] size_t get_loading_count() const;
        [[nodiscard]] size_t get_error_count() const;
        [[nodiscard]] size_t get_memory_usage() const;
        [[nodiscard]] IconQuality determine_icon_quality_from_size(
            const nlohmann::json& sizes_json);

        void update();

        // Utility
        static std::optional<std::string> icon_type_to_string(const IconType& type);
        static std::optional<IconType> string_to_icon_type(const std::string& str);

    private:
        struct CacheEntry
        {
            IconData data;
            IconConfig config;
            bool locked {false};

            [[nodiscard]] bool should_unload(const std::chrono::seconds seconds_to_unload) const
            {
                return !locked
                    && !config.persistent
                    && data.state != IconState::LOADING_IO
                    && data.state != IconState::LOADING_GPU
                    && (std::chrono::steady_clock::now() - data.last_access > seconds_to_unload);
            }
        };

        struct LoadTask
        {
            IconType type;
            std::shared_ptr<std::promise<bool>> promise;
            std::chrono::steady_clock::time_point start_time;
        };

        struct GPUUploadTask
        {
            IconType type;
            std::shared_ptr<c2l::core::resources::TextureResource> texture;
        };

        bool load_icon_internal(const IconType& type, bool async);
        void load_on_io_thread(const IconType& type);
        void upload_on_main_thread(const IconType& type);
        void load_default_icons();

        bool draw_icon_internal(
            const CacheEntry& entry,
            const ImVec2& size,
            const ImVec4& tint = {-1, 0, 0, 0},
            bool as_button = false,
            const char* button_id = nullptr,
            const char* tooltip = nullptr
        );

        // Thread-safe cache access
        std::shared_ptr<CacheEntry> get_or_create_entry(const IconType& type);
        std::shared_ptr<CacheEntry> find_entry(const IconType& type);
        std::shared_ptr<const CacheEntry> find_entry(const IconType& type) const;

        // State management
        void set_icon_state(const IconType& type, const IconState& state, float progress = 0.0f);
        void update_icon_progress(const IconType& type, float progress);

        // Error handling
        void handle_icon_error(const IconType&, const std::string& error);
        void retry_icon_load(const IconType& type);

        // Quality management
        void apply_quality_settings(
            const std::shared_ptr<c2l::core::resources::TextureResource>& texture,
            IconQuality quality);

        // Memory management
        void update_memory_usage(int64_t delta);
        void evict_old_icons();

        // Queue processing
        void process_gpu_queue();
        void process_futures();

        // Members
        c2l::core::ThreadManager& m_thread_manager;
        c2l::core::resources::ResourceManager& m_resource_manager;
        c2l::core::JsonConfigManager& m_json_config_manager;


        mutable std::shared_mutex m_cache_mutex;
        std::unordered_map<IconType, std::shared_ptr<CacheEntry>> m_cache;
        std::unordered_map<IconType, IconConfig> m_registry;

        // Loading state
        std::unordered_set<IconType> m_loading_icons;
        std::unordered_set<IconType> m_uploading_icons;
        std::mutex m_loading_mutex;

        // Task queues
        std::queue<GPUUploadTask> m_gpu_queue;
        std::mutex m_gpu_mutex;

        std::vector<std::pair<IconType, std::future<bool>>> m_io_futures;
        std::mutex m_futures_mutex;

        // Configuration
        size_t m_memory_limit               {256 * 1024 * 1024};
        std::atomic<size_t> m_memory_usage  {0};
        std::chrono::seconds m_cache_ttl    {300};  // for 5 minutes

        // Callback
        IconCallback m_loaded_callback;
        IconCallback m_error_callback;

        // Fallback texture
        static GLuint s_fallback_texture;
        static std::once_flag s_fallback_init_flag;
    };
}

#endif // CODE2LOGIC_ICON_MANAGER_HPP