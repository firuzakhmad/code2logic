#include "renderer.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace c2l::graphics
{

	Renderer::Renderer(core::GLFWWindow& window)
		: m_window{window}, m_imgui_manager{std::make_shared<ImGuiManager>(window)}
	{}
	
	Renderer::~Renderer()
	{}

	void Renderer::render()
	{
		m_imgui_manager->begin_frame();
	}
	
	void Renderer::clear()
	{
	    glViewport(
	    	0,
	    	0,
	    	m_window.get_size().x,
	    	m_window.get_size().y
		);

	    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_imgui_manager->end_frame();
	}

} // namespace c2l::graphics
