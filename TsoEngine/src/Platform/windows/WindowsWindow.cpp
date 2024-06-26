#include "TPch.h"
#include "glad/glad.h"
#include "WindowsWindow.h"

#include "Tso/Event/ApplicationEvent.h"
#include "Tso/Event/KeyEvent.h"
#include "Tso/Event/MouseEvent.h"
#include "Platform/OpenGL/OpenGLContext.h"


namespace Tso {
	static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error,const char* description){
    TSO_CORE_ERROR("error [{0}]: {1}",error,description);
}

	Window* Window::Create(const WindowProps& props) {
		return new WindowsWindow(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props) {
		Init(props);
	}

	WindowsWindow::~WindowsWindow(){
		ShutDown();
	}

	void WindowsWindow::ShutDown() {
		glfwDestroyWindow(m_Window);
	}

	void WindowsWindow::Init(const WindowProps& props) {
		m_Data.Title  = props.Title;
		m_Data.Width  = props.Width;
		m_Data.Height = props.Height;

		TSO_CORE_INFO("Create window {0} ({1} , {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized) {
			int success = glfwInit();
			TSO_CORE_ASSERT(success, "Could not initialize GLFW!");
            
            glfwSetErrorCallback(GLFWErrorCallback);
			s_GLFWInitialized = true;
		}
#ifdef TSO_PLATFORM_MACOSX
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#endif
		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
        m_Context = new OpenGLContext(m_Window);

        m_Context->Init();
  //	glfwMakeContextCurrent(m_Window);
  //    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		//set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
			});
        
        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
                {
                    WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
                    WindowCloseEvent event;
                    data.EventCallback(event);
                });
        
        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
                {
                    WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                    switch (action)
                    {
                        case GLFW_PRESS:
                        {
                            MouseButtonPressedEvent event(button);
                            data.EventCallback(event);
                            break;
                        }
                        case GLFW_RELEASE:
                        {
                            MouseButtonReleasedEvent event(button);
                            data.EventCallback(event);
                            break;
                        }
                    }
                });
        
        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
                {
                    WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                    switch (action)
                    {
                        case GLFW_PRESS:
                        {
                            KeyPressedEvent event(key, 0);
                            data.EventCallback(event);
                            break;
                        }
                        case GLFW_RELEASE:
                        {
                            KeyReleasedEvent event(key);
                            data.EventCallback(event);
                            break;
                        }
                        case GLFW_REPEAT:
                        {
                            KeyPressedEvent event(key, true);
                            data.EventCallback(event);
                            break;
                        }
                    }
                });
        
        glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
                {
                    WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                    KeyTypedEvent event(keycode);
                    data.EventCallback(event);
                });
        
        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
                {
                    WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                    MouseScrolledEvent event((float)xOffset, (float)yOffset);
                    data.EventCallback(event);
                });
        
        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
                {
                    WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                    MouseMovedEvent event((float)xPos, (float)yPos);
                    data.EventCallback(event);
                });
        
	}


	void WindowsWindow::OnUpdate()const {
		glfwPollEvents();
        m_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled) {
		if (enabled)
			glfwSwapInterval(1);//frame rate,1 means refresh every frame,while n mean refresh every n frame
		else
			glfwSwapInterval(0);//never refresh
	}

	bool WindowsWindow::IsVSync()const {
		return m_Data.VSync;
	} 


}
