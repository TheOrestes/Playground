
#include "PlaygroundPCH.h"
#include "PlaygroundHeaders.h"
#include "Application.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Application::Application(const std::string& _title, uint16_t _width, uint16_t _height)
    :   m_strWindowTitle(_title),
        m_uiWindowWidth(_width),
        m_uiWindowHeight(_height)
{
    m_pWindow = nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Application::~Application()
{
    Shutdown();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Application::Initialize()
{
    // Initialize GLFW
    if (!glfwInit())
    {
        LOG_CRITICAL("GLFW Initialization failed!");
        return false;
    }

    // GLFW Window parameters/settings
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create Window
    m_pWindow = glfwCreateWindow(m_uiWindowWidth, m_uiWindowHeight, m_strWindowTitle.c_str(), nullptr, nullptr);
    
    // check if creation successful or not? 
    if (!m_pWindow)
    {
        LOG_CRITICAL("{0} Window of size ({1}, {2}) creation Failed!", m_strWindowTitle, m_uiWindowWidth, m_uiWindowHeight);
        return false;
    }
    else
    {
        LOG_INFO("{0} Window of size ({1}, {2}) created successfully!", m_strWindowTitle, m_uiWindowWidth, m_uiWindowHeight);
    }

    // Set 
    glfwSetWindowUserPointer(m_pWindow, this);

    // Register Events! 
    glfwSetWindowCloseCallback(m_pWindow, EventWindowClosedCallback);
    glfwSetWindowSizeCallback(m_pWindow, EventWindowResizedCallback);
    glfwSetKeyCallback(m_pWindow, EventKeyHandlerCallback);
    glfwSetCursorPosCallback(m_pWindow, EventMousePositionCallback);
    glfwSetMouseButtonCallback(m_pWindow, EventMouseButtonCallback);
    glfwSetScrollCallback(m_pWindow, EventMouseScrollCallback);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::Run()
{
    if (Initialize())
    {
        MainLoop();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::MainLoop()
{
    float delta = 0.0f;
    float lastTime = 0.0f;

    while (!glfwWindowShouldClose(m_pWindow))
    {
        glfwPollEvents();

        float currTime = glfwGetTime();
        delta = currTime - lastTime;
        lastTime = currTime;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::Shutdown()
{
    glfwDestroyWindow(m_pWindow);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EventWindowClosedCallback(GLFWwindow* pWindow)
{
    LOG_DEBUG("Window Closed");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EventWindowResizedCallback(GLFWwindow* pWindow, int width, int height)
{
    LOG_DEBUG("Window Resized ({0},{1})", width, height);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EventKeyHandlerCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods)
{
    
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EventMousePositionCallback(GLFWwindow* pWindow, double xPos, double yPos)
{
    
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EventMouseButtonCallback(GLFWwindow* pWindow, int button, int action, int mods)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Application::EventMouseScrollCallback(GLFWwindow* pWindow, double xOffset, double yOffset)
{
}
