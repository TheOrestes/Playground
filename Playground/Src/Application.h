#pragma once

#include "PlaygroundPCH.h"
#include "GLFW/glfw3.h"

class Application
{
public:
	Application() {};
	Application(const std::string& _title, uint16_t _width, uint16_t _height);
		
	~Application();

	bool			Initialize();
	void			Run();
	void			Shutdown();

private:
	GLFWwindow*		m_pWindow;
	uint16_t		m_uiWindowWidth;
	uint16_t		m_uiWindowHeight;
	std::string		m_strWindowTitle;

	void			MainLoop();
};
