#ifndef I_SCENE_HPP
#define I_SCENE_HPP

/**
 * @brief Defines the interface for application scenes.
 *
 * A scene represents a self-contained state of the application
 * such as (Main Menu, Algorithm Visualizer, Comparison View).
 *
 * Design Rules:
 * - Scenes do NOT manage window, timer, or render loop
 * - Scenes do NOT directly switch scenes
 * - Scenes expose intent via callbacks
 *
 * Lifecycle is fully controlled by SceneManager.
 */

#include "algorithms/core/algorithm_registry.hpp"
#include "graphics/renderer.hpp"
#include "core/input_handler/input_handler.hpp"
#include "ui/managers/ui_manager.hpp"
#include "core/resources/resource_manager.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "ui/managers/icon_manager.hpp"
#include "core/json_config_manager/json_config_manager.hpp"

namespace c2l::scenes
{
    /**
     * @class IScene
     * @brief Base interface for all application scenes.
     *
     * A scene encapsulates input handling, simulation updates,
     * and rendering for a specific application state.
     *
     * Ownership:
     * - Owned by SceneManager
     * - Accessed by Application through SceneManager
     *
     * Threading:
     * - All scene methods are called from the main thread
     */
    class IScene
    {
    public:
        IScene(graphics::Renderer& renderer,
               core::ThreadManager& thread_manager,
               core::JsonConfigManager& json_config_manager,
               algorithms::AlgorithmRegistry& algorithm_registry,
               core::resources::ResourceManager& resource_manager,
               ui::managers::IconManager& icon_manager)
            : m_renderer{renderer}
            , m_thread_manager{thread_manager}
            , m_json_config_manager{json_config_manager}
            , m_algorithm_registry{algorithm_registry}
            , m_resource_manager{resource_manager}
            , m_icon_manager{icon_manager}
        {}
        virtual ~IScene() = default;

        /**
         * @brief Called once when the scene is first created.
         *
         * Use this to allocate persistent resources.
         *
         * @note Called exactly once during the scene's lifetime.
         */
        virtual void on_create()                                                        = 0;

        /**
         * @brief Called before the scene is permanently destroyed.
         *
         * Use this to release persistent resources.
         *
         * @note Called exactly once during shutdown.
         */
        virtual void on_destroy()                                                       = 0;

        /**
         * @brief Called when the scene becomes the active scene.
         *
         * Used to reset transient state such as animations or UI focus.
         *
         * @note Called once every time the scene is activated.
         */
        virtual void on_activate()                                                      = 0;

        /**
         * @brief Called when the scene is no longer active.
         *
         * Used to pause logic or save transient state.
         */
        virtual void on_deactivate()                                                    = 0;

        /**
         * @brief Processes user input.
         *
         * @param input Current frame input snapshot.
         *
         * @note Called once per frame before update().
         */
        virtual void process_input(const core::InputHandler& input)                     = 0;

        /**
         * @brief Advances scene simulation.
         *
         * @param dt Fixed timestep duration in seconds.
         *
         * @note Called zero or more times per frame depending on accumulated time.
         */
        virtual void update(double dt)                                                  = 0;

        /**
         * @brief Renders the scene.
         *
         * Rendering must be stateless with respect to simulation timing.
         */
        virtual void render()                                                           = 0;

        // UI management
        [[nodiscard]] virtual ui::managers::UIManager& get_ui_manager()                 = 0;
        [[nodiscard]] virtual const ui::managers::UIManager& get_ui_manager() const     = 0;
        // Scene state
        [[nodiscard]] bool is_created() const { return m_created; }

    protected:
        void mark_created() { m_created = true; }

        graphics::Renderer& m_renderer;
        core::ThreadManager& m_thread_manager;
        core::JsonConfigManager& m_json_config_manager;
        algorithms::AlgorithmRegistry& m_algorithm_registry;
        core::resources::ResourceManager& m_resource_manager;
        c2l::ui::managers::IconManager& m_icon_manager;

        bool m_created {false};
    };

} // namespace c2l::scenes

#endif //I_SCENE_HPP

