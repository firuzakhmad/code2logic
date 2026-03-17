//
// Created by Akhmad on 12/25/25.
//

#ifndef CODE2LOGIC_MAIN_MENU_SCENE_HPP
#define CODE2LOGIC_MAIN_MENU_SCENE_HPP

#include "scenes/scene_manager.hpp"
#include "scenes/base_scene.hpp"
#include "algorithms/core/algorithm_registry.hpp"

namespace c2l::scenes
{
    class MainMenuScene final : public BaseScene
    {
    public:
        MainMenuScene(graphics::Renderer& renderer,
            core::ThreadManager& thread_manager,
            core::JsonConfigManager& json_config_manager,
            algorithms::AlgorithmRegistry& algorithm_registry,
            core::resources::ResourceManager& resource_manager,
            ui::managers::IconManager& icon_manager
        );
        ~MainMenuScene() override = default;

        /**
         * @copydoc IScene::on_create()
         */
        void on_create() override;

        /**
         * @copydoc IScene::on_destroy()
         */
        void on_destroy() override;

        /**
         * @copydoc IScene::on_activate()
         */
        void on_activate() override;

        /**
         * @copydoc IScene::on_deactivate()
         */
        void on_deactivate() override;

        /**
         * @copydoc IScene::process_input()
         */
        void process_input(const core::InputHandler& input) override;

        /**
         * @copydoc IScene::update()
         */
        void update(double dt) override;

        /**
         * @copydoc IScene::render()
         */
        void render() override;

    private:
    };

} // namespace c2l::scenes

#endif //CODE2LOGIC_MAIN_MENU_SCENE_HPP
