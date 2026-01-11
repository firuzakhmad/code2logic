#include "icon_manager.hpp"
#include "core/utils/assert/assert.hpp"

#include <sys/stat.h>
#include <math.h>

namespace c2l::ui::managers
{
    GLuint IconManager::s_fallback_texture = 0;
    std::once_flag IconManager::s_fallback_init_flag;

    IconManager::IconManager(
        core::ThreadManager& thread_manager,
        core::resources::ResourceManager& resource_manager)
            : m_thread_manager{thread_manager}
            , m_resource_manager{resource_manager}
    {
        // Fallback initialization of the texture
        std::call_once(s_fallback_init_flag, [] ()
        {
            glGenTextures(1, &s_fallback_texture);
            glBindTexture(GL_TEXTURE_2D, s_fallback_texture);

            unsigned char pixels[] = {
                255, 0, 0, 255,     0, 255, 0, 255,
                0, 0, 255, 255,     255, 255, 255, 255
            };

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            LOG_DEBUG("IconManager fallback texture created");
        });

        load_default_icons();

        LOG_INFO("IconManager initialized");
    }

    IconManager::~IconManager()
    {
        unload_all(false);
    }

    bool IconManager::register_icon(const IconType& type, const IconConfig& config)
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        if (m_registry.find(type) != m_registry.end())
        {
            LOG_WARNING("Icon {} already registered", icon_type_to_string(type));
            return false;
        }

        m_registry[type] = config;

        // Creating cache entry if preload requested
        if (config.preload)
        {
            auto entry = get_or_create_entry(type);
            entry->config = config;

            lock.unlock();
            load_icon(type, config.preferred_thread_type == c2l::core::ThreadManager::ThreadType::IO);
        }

        LOG_DEBUG("Registered icon {}: {}", icon_type_to_string(type), config.path);

        return true;
    }

    bool IconManager::register_icons(const std::vector<std::pair<IconType, IconConfig>>& icons)
    {
        constexpr bool success = true;

        for (const auto& [type, config] : icons)
        {
            if (!register_icon(type, config))
            {
                return false;
            }
        }

        LOG_INFO("Registered icons {}", icons.size());

        return success;
    }

    bool IconManager::unregister_icon(const IconType &type)
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        // Removing from registry
        if (m_registry.erase(type) == 0)
        {
            return false;
        }

        // Unloading from cache
        auto it = m_cache.find(type);
        if (it != m_cache.end())
        {
            if (it->second->data.resource)
            {
                m_memory_usage -= it->second->data.memory_usage;
                it->second->data.resource->unload();
            }
            m_cache.erase(it);
        }

        LOG_DEBUG("Unregistered icon {}", icon_type_to_string(type));
        return true;
    }

    void IconManager::load_icon(const IconType& type, bool async)
    {
        load_icon_internal(type, async);
    }

    void IconManager::load_icons(const std::vector<IconType> &icons)
    {
        for (auto type : icons)
        {
            load_icon(type, true);
        }
    }

    void IconManager::preload_all()
    {
        std::vector<IconType> icons;

        {
            std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

            icons.reserve(m_registry.size());
            for (const auto& [type, config] : m_registry)
            {
                icons.push_back(type);
            }
        }

        load_icons(icons);
    }

    bool IconManager::load_icon_internal(const IconType &type, bool async)
    {
        {
            std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

            std::shared_ptr<CacheEntry> entry = find_entry(type);

            if (!entry)
            {
                // Checking if it exists in the registry
                auto registry_it = m_registry.find(type);
                if (registry_it == m_registry.end())
                {
                    LOG_ERROR("Icon {} not registered", icon_type_to_string(type));
                    return false;
                }

                lock.unlock();
                std::unique_lock<std::shared_mutex> unique_lock(m_cache_mutex);

                entry = get_or_create_entry(type);
                entry->config = registry_it->second;
            }
            else if (entry->data.is_ready())
            {
                entry->data.last_access = std::chrono::steady_clock::now();
                return true;
            }
            else if (entry->data.is_loading())
            {
                return false;
            }
        }

        // Setting loading state
        set_icon_state(type, IconState::LOADING_IO, 0.1f);

        if (async)
        {
            // Schedule on IO thread
            m_thread_manager.enqueue_task(
                c2l::core::ThreadManager::ThreadType::IO,
                [this, type]()
                {
                    load_on_io_thread(type);
                }
            );
        }
        else {
            // Loading synchronously
            load_on_io_thread(type);
            upload_on_main_thread(type);
        }

        return true;
    }

    void IconManager::load_on_io_thread(const IconType &type)
    {
        IconConfig config;

        {
            std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

            const std::shared_ptr<CacheEntry> entry = find_entry(type);
            if (!entry)
            {
                handle_icon_error(type, "Entry not found");
                return;
            }

            config = entry->config;
        }

        try
        {
            update_icon_progress(type, 0.3f);

            // Loading texture resource
            const auto resource = m_resource_manager.load<c2l::core::resources::TextureResource>(config.path);
            if (!resource)
            {
                LOG_ERROR("Failed to create texture resource for icon {}", icon_type_to_string(type));
            }

            // Loading data only (noGPU upload)
            if (!resource->load_data_only())
            {
                LOG_ERROR("Failed to load texture data for icon {}", icon_type_to_string(type));
            }

            update_icon_progress(type, 0.6f);

            // Updating entry
            {
                std::unique_lock<std::shared_mutex> unique_lock(m_cache_mutex);

                if (const auto e = find_entry(type); e)
                {
                    e->data.resource = resource;
                    e->data.size = ImVec2(
                        static_cast<float>(resource->get_width()),
                        static_cast<float>(resource->get_height())
                    );
                    e->data.memory_usage = resource->get_memory_usage();
                    m_memory_usage += e->data.memory_usage;
                }
            }

            set_icon_state(type, IconState::LOADING_GPU, 0.8f);

            // Queueing for GPU upload
            {
                std::lock_guard<std::mutex> lock(m_gpu_mutex);
                m_gpu_queue.push({type, resource});
            }

            LOG_DEBUG("IO Thread: Icon {} data loaded ({}x{})",
                     icon_type_to_string(type),
                     resource->get_width(),
                     resource->get_height());

        } catch (const std::exception& e)
        {
            handle_icon_error(type, e.what());
        }
    }

    void IconManager::upload_on_main_thread(const IconType &type)
    {
        std::shared_ptr<CacheEntry> entry;
        std::shared_ptr<c2l::core::resources::TextureResource> resource;

        {
            std::shared_lock<std::shared_mutex> lock(m_cache_mutex);
            entry = find_entry(type);
            if (!entry || !entry->data.resource)
            {
                handle_icon_error(type, "No texture for GPU upload");
                return;
            }
            resource = entry->data.resource;
        }

        try
        {
            update_icon_progress(type, 0.9f);

            // Uploading to GPU (MUST be on main thread)
            if (!resource->upload_to_gpu())
            {
                LOG_ERROR("GPU upload failed for icon {}", icon_type_to_string(type));
            }

            // Applying quality settings
            apply_quality_settings(resource, entry->config.quality);

            // Updating entry
            {
                std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

                if (const auto e = find_entry(type); e)
                {
                    e->data.texture_id = resource->get_texture_id();
                    e->data.gl_texture_id = resource->get_gl_texture_id();
                    e->data.state = IconState::READY;
                    e->data.progress = 1.0f;
                    e->data.load_time = std::chrono::steady_clock::now();
                    e->data.last_access = std::chrono::steady_clock::now();
                }
            }

            LOG_INFO("Icon {} ready ({}x{}, {}KB)",
                    icon_type_to_string(type),
                    resource->get_width(),
                    resource->get_height(),
                    resource->get_memory_usage() / 1024);

            // Calling callback
            if (m_loaded_callback)
            {
                try
                {
                    m_loaded_callback(type);
                } catch (...)
                {
                    // Ignore callback errors
                }
            }

        } catch (const std::exception& e)
        {
            handle_icon_error(type, e.what());
        }
    }

    void IconManager::load_default_icons()
    {
        const std::vector<std::pair<IconType, IconConfig>>& icons = {
        { IconType::PLAY, {"resources/icons/play.png", IconQuality::HIGH} },
        { IconType::STEP_FORWARD,{"resources/icons/arrow_right.png", IconQuality::HIGH} },
        { IconType::STEP_BACKWARD,{"resources/icons/arrow_left.png", IconQuality::HIGH} },
        { IconType::PAUSE,{"resources/icons/pause.png", IconQuality::HIGH} },
        { IconType::ARRAY,{"resources/icons/array.png", IconQuality::HIGH} },
        { IconType::ALGORITHM,{"resources/icons/algorithm.png", IconQuality::HIGH} },
        { IconType::STATISTIC,{"resources/icons/statistic.png", IconQuality::HIGH} },
        { IconType::STEPS,{"resources/icons/steps.png", IconQuality::HIGH} },
        { IconType::RESET,{"resources/icons/reset.png", IconQuality::HIGH} }
        };

        register_icons(icons);
    }


    bool IconManager::draw_icon_internal(
        const CacheEntry& entry,
        const ImVec2& size,
        const ImVec4& tint,
        bool as_button,
        const char* button_id,
        const char* tooltip)
    {
        if (entry.data.texture_id == 0)
            return false;

        // Resolving display size
        ImVec2 display_size = size;
        if (display_size.x <= 0 && display_size.y <= 0)
        {
            display_size = (entry.data.size.x > 0 && entry.data.size.y > 0)
                ? entry.data.size
                : DEFAULT_BUTTON_ICON_SIZE;
        }

        const ImVec4 actual_tint =
            (tint.x < 0) ? ImVec4(1, 1, 1, 1) : tint;

        bool clicked = false;

        if (as_button)
        {
            clicked = ImGui::ImageButton(
                button_id,
                entry.data.texture_id,
                display_size,
                entry.config.uv0,
                entry.config.uv1,
                ImVec4(0, 0, 0, 0), // bg
                actual_tint);
        }
        else
        {
            ImGui::Image(
                entry.data.texture_id,
                display_size,
                entry.config.uv0,
                entry.config.uv1);
        }

        if (tooltip && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(tooltip);
            ImGui::EndTooltip();
        }

        return clicked;
    }

    bool IconManager::render_icon(
        const IconType& type,
        const ImVec2& size)
    {
        std::shared_ptr<CacheEntry> entry;

        {
            std::shared_lock lock(m_cache_mutex);
            entry = find_entry(type);
            if (!entry)
            {
                lock.unlock();
                load_icon(type, true);
                render_loading_indicator(size);
                return false;
            }
        }

        entry->data.last_access = std::chrono::steady_clock::now();

        switch (entry->data.state)
        {
            case IconState::READY:
                return draw_icon_internal(
                    *entry, size);

            case IconState::LOADING_IO:
            case IconState::LOADING_GPU:
                render_loading_indicator(size);
                return false;

            case IconState::FAILED:
                render_error_icon(size);
                if (entry->data.retry_count < entry->config.max_retries)
                    retry_icon_load(type);
                return false;

            default:
                render_loading_indicator(size);
                return false;
        }
    }


    bool IconManager::render_icon_button(
        const char* str_id,
        const IconType& type,
        const ImVec2& size,
        const ImVec4& tint,
        const char* tooltip)
    {
        std::shared_ptr<CacheEntry> entry;

        {
            std::shared_lock lock(m_cache_mutex);
            entry = find_entry(type);
            if (!entry)
            {
                lock.unlock();
                load_icon(type, true);
                render_loading_indicator(size);
                return false;
            }
        }

        entry->data.last_access = std::chrono::steady_clock::now();

        switch (entry->data.state)
        {
            case IconState::READY:
                return draw_icon_internal(
                    *entry, size, tint,
                    true, str_id, tooltip);

            case IconState::LOADING_IO:
            case IconState::LOADING_GPU:
                render_loading_indicator(size);
                return false;

            case IconState::FAILED:
                render_error_icon(size);
                if (entry->data.retry_count < entry->config.max_retries)
                    retry_icon_load(type);
                return false;

            default:
                render_loading_indicator(size);
                return false;
        }
    }



    void IconManager::render_loading_indicator(const ImVec2& size)
    {
        ImVec2 actual_size = size;
        if (actual_size.x <= 0 || actual_size.y <= 0)
        {
            actual_size = ImVec2(32, 32);
        }

        // Simple spinner
        ImVec2 center = ImVec2(
            ImGui::GetCursorScreenPos().x + actual_size.x * 0.5f,
            ImGui::GetCursorScreenPos().y + actual_size.y * 0.5f
        );

        float radius = std::min(actual_size.x, actual_size.y) * 0.3f;
        float time = static_cast<float>(ImGui::GetTime());

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        for (int i = 0; i < 8; ++i)
        {
            float angle = time * 3.14159f * 2.0f + (i * 3.14159f / 4.0f);
            ImVec2 pos = ImVec2(
                center.x + cosf(angle) * radius,
                center.y + sinf(angle) * radius
            );

            draw_list->AddCircleFilled(pos, radius * 0.2f,
                                      ImColor(100, 100, 255, 200));
        }

        // Advance cursor
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + actual_size.x);
    }

    void IconManager::render_error_icon(const ImVec2& size, const char* text)
    {
        ImVec2 actual_size = size;
        if (actual_size.x <= 0 || actual_size.y <= 0)
        {
            actual_size = ImVec2(32, 32);
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();

        // Draw error background
        draw_list->AddRectFilled(pos, ImVec2(pos.x + actual_size.x, pos.y + actual_size.y),
                                ImColor(255, 50, 50, 100));

        // Draw X
        float padding = actual_size.x * 0.2f;
        draw_list->AddLine(ImVec2(pos.x + padding, pos.y + padding),
                          ImVec2(pos.x + actual_size.x - padding, pos.y + actual_size.y - padding),
                          ImColor(255, 255, 255, 255), 2.0f);
        draw_list->AddLine(ImVec2(pos.x + actual_size.x - padding, pos.y + padding),
                          ImVec2(pos.x + padding, pos.y + actual_size.y - padding),
                          ImColor(255, 255, 255, 255), 2.0f);

        // Advance cursor
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + actual_size.x);
    }

    void IconManager::update()
    {
        // Processing GPU upload queue
        process_gpu_queue();

        // Checking memory usage
        if (static_cast<double>(m_memory_usage) > static_cast<double>(m_memory_limit) * 0.8)
        {
            evict_old_icons();
        }

        cleanup_cache();
    }

    void IconManager::process_gpu_queue()
    {
        std::unique_lock<std::mutex> lock(m_gpu_mutex);

        while (!m_gpu_queue.empty())
        {
            GPUUploadTask task = m_gpu_queue.front();
            m_gpu_queue.pop();

            lock.unlock();
            upload_on_main_thread(task.type);
            lock.lock();
        }
    }

    std::shared_ptr<IconManager::CacheEntry> IconManager::get_or_create_entry(const IconType& type)
    {
        const auto it = m_cache.find(type);
        if (it == m_cache.end())
        {
            auto entry = std::make_shared<CacheEntry>();
            entry->data.type = type;
            entry->data.state = IconState::UNLOADED;
            entry->data.last_access = std::chrono::steady_clock::now();

            m_cache[type] = entry;
            return entry;
        }
        return it->second;
    }

    std::shared_ptr<IconManager::CacheEntry> IconManager::find_entry(const IconType& type)
    {
        const auto it = m_cache.find(type);
        return it != m_cache.end() ? it->second : nullptr;
    }

    std::shared_ptr<const IconManager::CacheEntry> IconManager::find_entry(const IconType& type) const
    {
        const auto it = m_cache.find(type);
        return it != m_cache.end() ? it->second : nullptr;
    }

    void IconManager::set_icon_state(const IconType& type, const IconState& state, float progress)
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        if (const auto entry = find_entry(type); entry)
        {
            entry->data.state = state;
            entry->data.progress = progress;
        }
    }

    void IconManager::update_icon_progress(const IconType& type, float progress)
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        if (const auto entry = find_entry(type); entry)
        {
            entry->data.progress = progress;
        }
    }

    void IconManager::handle_icon_error(const IconType& type, const std::string& error)
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        if (const auto entry = find_entry(type); entry)
        {
            entry->data.state = IconState::FAILED;
            entry->data.error_message = error;
            entry->data.retry_count++;

            LOG_ERROR("Icon {} error: {} (retry {}/{})",
                     icon_type_to_string(type), error,
                     entry->data.retry_count, entry->config.max_retries);

            if (m_error_callback)
            {
                try
                {
                    m_error_callback(type);
                } catch (...)
                {
                    // Ignore callback errors
                }
            }
        }
    }

    void IconManager::retry_icon_load(const IconType& type)
    {
        {
            std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

            std::shared_ptr<CacheEntry> entry = find_entry(type);
            if (!entry || entry->data.retry_count >= entry->config.max_retries)
            {
                return;
            }
        }

        LOG_DEBUG("Retrying icon {}", icon_type_to_string(type));
        load_icon(type, true);
    }

    void IconManager::apply_quality_settings(
        const std::shared_ptr<c2l::core::resources::TextureResource>& texture,
        IconQuality quality)
    {
        if (!texture) return;

        switch (quality)
        {
            case IconQuality::LOW:
                texture->set_filtering(GL_NEAREST, GL_NEAREST);
                break;
            case IconQuality::MEDIUM:
                texture->set_filtering(GL_LINEAR, GL_LINEAR);
                break;
            case IconQuality::HIGH:
                texture->set_filtering(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                texture->generate_mipmaps();
                break;
            case IconQuality::ULTRA:
                texture->set_filtering(GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
                texture->generate_mipmaps();
                texture->set_wrapping(GL_REPEAT, GL_REPEAT);
                break;
        }
    }

    void IconManager::update_memory_usage(int64_t delta)
    {
        m_memory_usage += static_cast<size_t>(delta);
    }

    void IconManager::evict_old_icons()
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        LOG_DEBUG("Evicting old icons");
        std::vector<IconType> to_unload;
        auto now = std::chrono::steady_clock::now();

        for (const auto& [type, entry] : m_cache)
        {
            LOG_DEBUG("icon {}", icon_type_to_string(type));

            if (!entry->config.persistent &&
                !entry->locked &&
                entry->data.state == IconState::READY &&
                (now - entry->data.last_access) > std::chrono::seconds(60))
            {
                to_unload.push_back(type);
            }
        }

        for (auto type : to_unload) {
            unload_icon(type);
        }

        if (!to_unload.empty()) {
            LOG_DEBUG("Evicted {} old icons", to_unload.size());
        }
    }

    bool IconManager::is_ready(const IconType& type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        auto entry = find_entry(type);
        return entry && entry->data.is_ready();
    }

    bool IconManager::is_loading(const IconType& type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        auto entry = find_entry(type);
        return entry && entry->data.is_loading();
    }

    IconState IconManager::get_state(const IconType& type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        auto entry = find_entry(type);
        return entry ? entry->data.state : IconState::UNLOADED;
    }

    float IconManager::get_progress(const IconType& type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        auto entry = find_entry(type);
        return entry ? entry->data.progress.load() : 0.0f;
    }

    ImTextureID IconManager::get_texture_id(const IconType& type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        auto entry = find_entry(type);
        return entry ? entry->data.texture_id : 0;
    }

    ImVec2 IconManager::get_size(const IconType& type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);

        auto entry = find_entry(type);
        return entry ? entry->data.size : ImVec2(0, 0);
    }

    void IconManager::unload_icon(const IconType& type)
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        auto it = m_cache.find(type);
        if (it != m_cache.end())
        {
            auto& entry = it->second;
            if (entry->data.resource)
            {
                m_memory_usage -= entry->data.memory_usage;
                entry->data.resource->unload();
            }
            m_cache.erase(it);
        }
    }

    void IconManager::unload_all(bool keep_persistent)
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        for (auto it = m_cache.begin(); it != m_cache.end();)
        {
            if (!keep_persistent || !it->second->config.persistent)
            {
                if (it->second->data.resource)
                {
                    m_memory_usage -= it->second->data.memory_usage;
                    it->second->data.resource->unload();
                }
                it = m_cache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void IconManager::reload_icon(const IconType& type)
    {
        unload_icon(type);
        load_icon(type, true);
    }

    void IconManager::process_futures()
    {}

    void IconManager::cleanup_cache()
    {
        std::unique_lock<std::shared_mutex> lock(m_cache_mutex);

        for (auto it = m_cache.begin(); it != m_cache.end();)
        {
            if (it->second->should_unload(m_cache_ttl))
            {
                if (it->second->data.resource)
                {
                    m_memory_usage -= it->second->data.memory_usage;
                    it->second->data.resource->unload();
                }
                it = m_cache.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void IconManager::set_memory_limit(size_t megabytes)
    {
        m_memory_limit = megabytes * 1024 * 1024;
    }

    void IconManager::set_loaded_callback(IconCallback callback)
    {
        m_loaded_callback = std::move(callback);
    }

    void IconManager::set_error_callback(IconCallback callback)
    {
        m_error_callback = std::move(callback);
    }

    size_t IconManager::get_loaded_count() const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);
        size_t count = 0;
        for (const auto& [_, entry] : m_cache)
        {
            if (entry->data.is_ready())
            {
                count++;
            }
        }
        return count;
    }

    size_t IconManager::get_loading_count() const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);
        size_t count = 0;
        for (const auto& [_, entry] : m_cache)
        {
            if (entry->data.is_loading())
            {
                count++;
            }
        }
        return count;
    }

    size_t IconManager::get_error_count() const
    {
        std::shared_lock<std::shared_mutex> lock(m_cache_mutex);
        size_t count = 0;
        for (const auto& [_, entry] : m_cache)
        {
            if (entry->data.state == IconState::FAILED)
            {
                count++;
            }
        }
        return count;
    }

    size_t IconManager::get_memory_usage() const
    {
        return m_memory_usage;
    }

    std::string IconManager::icon_type_to_string(const IconType& type)
    {
        static const char* names[] = {
            "PLAY", "PAUSE", "STEP_FORWARD", "STEP_BACKWARD", "RESET", "SETTINGS",
            "EXPAND", "COLLAPSE", "CLOSE", "INFO", "WARNING", "ERROR", "SUCCESS",
            "MENU", "GRID", "LIST", "SEARCH", "FILTER", "DOWNLOAD", "UPLOAD",
            "SAVE", "TRASH", "EDIT", "COPY", "PASTE", "UNDO", "REDO", "ARRAY",
            "ALGORITHM", "STATISTIC", "STEPS", "UNKNOWN"
        };

        size_t index = static_cast<size_t>(type);
        if (index < sizeof(names) / sizeof(names[0])) {
            return names[index];
        }
        return "UNKNOWN";
    }

    IconType IconManager::string_to_icon_type(const std::string& str)
    {
        static const char* names[] = {
            "PLAY", "PAUSE", "STEP_FORWARD", "STEP_BACKWARD", "RESET", "SETTINGS",
            "EXPAND", "COLLAPSE", "CLOSE", "INFO", "WARNING", "ERROR", "SUCCESS",
            "MENU", "GRID", "LIST", "SEARCH", "FILTER", "DOWNLOAD", "UPLOAD",
            "SAVE", "TRASH", "EDIT", "COPY", "PASTE", "UNDO", "REDO", "ARRAY",
            "ALGORITHM", "STATISTIC", "STEPS","UNKNOWN"
        };

        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        {
            if (names[i] == str) {
                return static_cast<IconType>(i);
            }
        }
        return IconType::UNKNOWN;
    }

} // namespace c2l::ui::managers