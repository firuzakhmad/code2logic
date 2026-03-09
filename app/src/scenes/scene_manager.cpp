#include "scenes/scene_manager.hpp"
#include "scenes/algorithm_comparison_scene.hpp"
#include "scenes/algorithm_visualizer_scene.hpp"
#include "scenes/base_scene.hpp"
#include "scenes/main_menu_scene.hpp"
#include "core/utils/assert/assert.hpp"

#include <imgui.h>

namespace c2l::scenes
{
    SceneManager::SceneManager(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::JsonConfigManager& json_config_manager,
        core::resources::ResourceManager& resource_manager,
        ui::managers::IconManager& icon_manager)
            : m_renderer{renderer}
            , m_thread_manager{thread_manager}
            , m_json_config_manager{json_config_manager}
            , m_resource_manager{resource_manager}
            , m_icon_manager{icon_manager}
    {
        initialize_scenes();
    }

    SceneManager::~SceneManager()
    {
    }

    void SceneManager::initialize_scenes()
    {
        const auto algorithm_scene = std::make_shared<scenes::AlgorithmVisualizerScene>(
            m_renderer,
            m_thread_manager,
            m_json_config_manager,
            m_resource_manager,
            m_icon_manager);

        // Create main menu
        const auto main_menu_scene = std::make_shared<scenes::MainMenuScene>(
            m_renderer,
            m_thread_manager,
            m_json_config_manager,
            m_resource_manager,
            m_icon_manager);


        const auto algorithm_comparison_scene = std::make_shared<scenes::AlgorithmComparisonScene>(
            m_renderer,
            m_thread_manager,
            m_json_config_manager,
            m_resource_manager,
            m_icon_manager);

        register_scene(SceneType::MAIN_MENU, main_menu_scene);
        register_scene(SceneType::ALGORITHM_VISUALIZER, algorithm_scene);
        register_scene(SceneType::ALGORITHM_COMPARISON, algorithm_comparison_scene);

        push_scene(SceneType::MAIN_MENU);
    }


    void SceneManager::register_scene(const SceneType& scene_type, std::shared_ptr<IScene> scene)
    {
        if (m_scenes.find(scene_type) != m_scenes.end())
        {
            LOG_WARNING("Scene '{}' already exists, replacing", scene_type_to_string(scene_type));
        }

        // Casting to BaseScene to set callbacks for switching between scenes
        if (auto base_scene = std::dynamic_pointer_cast<BaseScene>(scene))
        {
            base_scene->set_navigation_callbacks(
                [this](const SceneType& target_scene)
                {
                    this->push_scene(target_scene);
                },
                [this](const SceneType& target_scene)
                {
                    this->switch_to(target_scene);
                },
                [this]()
                {
                    this->pop_scene();
                }
            );
        }

        m_scenes[scene_type] = std::move(scene);
    }

    void SceneManager::unregister_scene(const SceneType& scene_type)
    {
        auto it = m_scenes.find(scene_type);
        if (it != m_scenes.end())
        {
            if (!m_stack.empty() && m_stack.top() == it->second)
            {
                pop_scene_immediate();
            }
            m_scenes.erase(it);
        }
    }

    void SceneManager::push_scene(const SceneType& scene_type)
    {
        auto it = m_scenes.find(scene_type);
        if (it == m_scenes.end())
        {
            LOG_ERROR("Scene {} not found", scene_type_to_string(scene_type));
            return;
        }

        // Deactivating current scene to push the new scene
        if (!m_stack.empty())
        {
            m_stack.top()->on_deactivate();
        }

        m_stack.push(it->second);
        m_stack.top()->on_activate();
    }

    void SceneManager::pop_scene()
    {
        if (m_stack.size() <= 1)
        {
            LOG_WARNING("Cannot pop the last scene");
            return;
        }

        m_stack.top()->on_deactivate();
        m_stack.pop();

        m_stack.top()->on_activate();
    }

    void SceneManager::switch_to(const SceneType& scene_type)
    {
        auto it = m_scenes.find(scene_type);
        if (it == m_scenes.end())
        {
            LOG_ERROR("Scene {} not found", scene_type_to_string(scene_type));
            return;
        }

        if (!m_stack.empty() && m_stack.top() != it->second)
        {
            if (!m_stack.empty())
            {
                m_stack.top()->on_deactivate();
                m_stack.pop();
            }

            m_stack.push(it->second);
            m_stack.top()->on_activate();
        }
    }


    void SceneManager::push_scene_immediate(const SceneType& scene_type)
    {
        auto it = m_scenes.find(scene_type);
        if (it == m_scenes.end()) return;

        if (!m_stack.empty())
            m_stack.top()->on_deactivate();

        m_stack.push(it->second);
        m_stack.top()->on_activate();
    }

    void SceneManager::pop_scene_immediate()
    {
        if (m_stack.size() <= 1) return;

        m_stack.top()->on_deactivate();
        m_stack.pop();
        m_stack.top()->on_activate();
    }

    void SceneManager::process_input(const core::InputHandler& input)
    {
        if (m_stack.empty()) return;

        m_stack.top()->process_input(input);
    }

    void SceneManager::update(double delta_time)
    {
        if (m_stack.empty()) return;

        m_stack.top()->update(delta_time);
    }

    void SceneManager::render()
    {
        if (m_stack.empty()) return;

        m_stack.top()->render();
    }

    [[nodiscard]] IScene& SceneManager::get_current_scene() const
    {
        C2L_ASSERT(!m_stack.empty(), "No scene in stack");
        return *m_stack.top();
    }

    [[nodiscard]] SceneType SceneManager::get_current_scene_name() const
    {
        for (const auto& [scene_type, scene] : m_scenes)
        {
            if (!m_stack.empty() && scene == m_stack.top())
            {
                return scene_type;
            }
        }
        return SceneType::UNKNOWN;
    }

    std::string SceneManager::scene_type_to_string(const SceneType& type)
    {
        static const char* names[] = {
            "MAIN_MENU",
            "ALGORITHM_VISUALIZER",
            "ALGORITHM_COMPARISON",
            "UNKNOWN"
        };

        size_t index = static_cast<size_t>(type);
        if (index < sizeof(names) / sizeof(names[0])) {
            return names[index];
        }
        return "UNKNOWN";
    }

    SceneType SceneManager::string_to_scene_type(const std::string& str)
    {
        static const char* names[] = {
            "MAIN_MENU",
            "ALGORITHM_VISUALIZER",
            "ALGORITHM_COMPARISON",
            "UNKNOWN"
        };

        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        {
            if (names[i] == str) {
                return static_cast<SceneType>(i);
            }
        }
        return SceneType::UNKNOWN;
    }


}
