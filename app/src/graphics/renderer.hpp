#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "core/window/glfw_window.hpp"
#include "graphics/imgui_manager.hpp"

#include <memory>

namespace c2l::graphics
{
	/**
	 * @brief OpenGL rendering system
	 * 
	 * Provides rendering capabilities using an existing GLFWWindow's OpenGL context.
	 * The class maintains a reference to the window but does not own it.
	 * 
	 * @note The associated GLFWWindow must outlive this Renderer instance
	 * @see GLFWWindow
	 */
	class Renderer
	{
	public:
	    /**
	     * @brief Construct a Renderer for the given window
	     * @param window Reference to an existing GLFWWindow
	     * @pre The window must have a valid OpenGL context
	     * 
	     * @details The renderer will use the window's OpenGL context for
	     * all rendering operations. The window must remain valid for the
	     * lifetime of the Renderer.
	     */
		explicit Renderer(core::GLFWWindow& window);
		~Renderer();

		/**
	     * @brief Gets the associated window
	     * @return Reference to the GLFWWindow used for rendering
	     */
		[[nodiscard]] core::GLFWWindow& get_window() const noexcept
		{
			return m_window;
		}

		[[nodiscard]] std::weak_ptr<ImGuiManager> get_imgui_manager() const noexcept
		{
			return m_imgui_manager;
		}

		void set_model_matrix(const glm::mat4& model) { m_model = model; }
		void set_view_matrix(const glm::mat4& view) { m_view = view; }
		void set_projection_matrix(const glm::mat4& projection) { m_projection = projection; }
    	void set_camera_position(const glm::vec3& camera_position) { m_camera_position = camera_position; }
    	void set_camera_front(const glm::vec3& camera_front) { m_camera_front = camera_front; }

		void render();
		void clear();


	private:
		core::GLFWWindow& m_window;
		std::shared_ptr<ImGuiManager> m_imgui_manager;


		glm::mat4 m_model		{1.0f};
		glm::mat4 m_view		{1.0f};
		glm::mat4 m_projection	{1.0f};

		glm::vec3 m_camera_position	{1.0f};
		glm::vec3 m_camera_front	{1.0f};


		// cube positions
		std::vector<glm::vec3> m_cube_positions;
		std::vector<glm::vec3> m_point_light_positions;

	};

} // namespace c2l::graphics

#endif // RENDERER_HPP