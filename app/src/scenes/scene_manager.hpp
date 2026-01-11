#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "i_scene.hpp"
#include <memory>
#include <functional>
#include <unordered_map>
#include <stack>
#include <vector>

#include "ui/managers/icon_manager.hpp"

namespace c2l::core
{
    class InputHandler;
}

namespace c2l::scenes
{
    enum class SceneType : uint8_t
    {
        MAIN_MENU,
        ALGORITHM_VISUALIZER,
        ALGORITHM_COMPARISON,

        UNKNOWN
    };

    /**
     * @class SceneManager
     * @brief Manages scene ownership, lifecycle, and navigation.
     *
     * SceneManager owns all scene instances and controls a stack-based
     * navigation model where scenes can be pushed, popped, or replaced.
     *
     * Responsibilities:
     * - Owns and registers all scene instances
     * - Manages scene activation and deactivation
     * - Routes input, update, and render calls to the top scene
     *
     * Design Constraints:
     * - Scenes must not directly modify the scene stack
     * - SceneManager is the sole authority for scene transitions
     *
     * Navigation Model:
     * - push_scene(): overlays a new scene
     * - pop_scene(): returns to the previous scene
     * - switch_to(): replaces the current scene
     *
     * Threading:
     * - Must be used from the main thread only
     */
    class SceneManager
    {
    public:
        SceneManager(
            graphics::Renderer& renderer,
            core::ThreadManager& thread_manager,
            core::resources::ResourceManager& resource_manager,
            ui::managers::IconManager& icon_manager);
        ~SceneManager();

        /**
         * @brief Creates and registers all application scenes.
         *
         * @note Called once during application startup.
         */
        void initialize_scenes();

        // Scene management
        /**
        * @brief Registers a scene instance.
        *
        * @param scene_type Scene identifier.
        * @param scene Scene instance to register.
        *
        * @note Ownership is transferred to SceneManager.
        */
        void register_scene(const SceneType& scene_type, std::shared_ptr<IScene> scene);
        void unregister_scene(const SceneType& scene_type);

        // Navigation with transitions
        void push_scene(const SceneType& scene_type);
        void pop_scene();
        void switch_to(const SceneType& scene_type);
        
        // Direct navigation (immediate)
        void push_scene_immediate(const SceneType& scene_type);
        void pop_scene_immediate();
        
        // Scene lifecycle
        void process_input(const core::InputHandler& input);
        void update(double delta_time);
        void render();

        // Information
        [[nodiscard]] IScene& get_current_scene() const;
        [[nodiscard]] SceneType get_current_scene_name() const;
        [[nodiscard]] size_t get_scene_count() const { return m_stack.size(); }

        static std::string scene_type_to_string(const SceneType& type);
        static SceneType string_to_scene_type(const std::string& str);

    private:
        graphics::Renderer& m_renderer;
        core::ThreadManager& m_thread_manager;
        core::resources::ResourceManager& m_resource_manager;
        ui::managers::IconManager& m_icon_manager;

        std::unordered_map<SceneType, std::shared_ptr<IScene>> m_scenes;
        std::stack<std::shared_ptr<IScene>> m_stack;
    };

} // namespace c2l::scenes

#endif // SCENE_MANAGER_HPP