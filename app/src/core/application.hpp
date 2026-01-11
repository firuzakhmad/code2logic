#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "core/utils/timer/timer.hpp"
#include "core/window/glfw_window.hpp"
#include "core/input_handler/input_handler.hpp"
#include "scenes/scene_manager.hpp"
#include "core/file_system/std_file_system.hpp"
#include "core/resources/resource_manager.hpp"
#include "ui/managers/icon_manager.hpp"

#include <memory>

namespace c2l
{
	class Application
	{
	public:
		Application();
		~Application();
		
		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void run();
		void initialize_services();

	private:
		std::unique_ptr<core::Timer> m_timer;
		std::unique_ptr<core::GLFWWindow> m_window;
		std::unique_ptr<core::InputHandler> m_input_handler;
		std::unique_ptr<graphics::Renderer> m_renderer;

		std::unique_ptr<core::ThreadManager> m_thread_manager;
		std::unique_ptr<core::filesystem::IFileSystem> m_file_system;
		std::unique_ptr<core::resources::ResourceManager> m_resource_manager;
		std::unique_ptr<c2l::ui::managers::IconManager> m_icon_manager;
		std::unique_ptr<scenes::SceneManager> m_scene_manager;
	};

} // namespace c2l

#endif // APPLICATION_HPP