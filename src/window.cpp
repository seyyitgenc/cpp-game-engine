#include <headers/window.h>

Window::Window()
{
}

Window::~Window()
{
    SDL_DestroyRenderer(m_renderer);
    m_renderer = NULL;
    SDL_DestroyWindow(m_window);
    m_window = NULL;
}

bool Window::Init(const char *title, int x, int y, int flag)
{
    if (m_fullscreen)
        flag = flag | SDL_WINDOW_FULLSCREEN;
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
    {
        std::cout << "Subsystems Initialized!" << std::endl;
        m_window = SDL_CreateWindow(title, x, y, m_screenWidth, m_screenHeight, flag);
        if (m_window != NULL)
        {
            std::cout << "Window Created!" << std::endl;
            SDL_SetWindowMinimumSize(m_window, 200, 200);
        }
        else
        {
            std::cout << "Window Creation Failed\n"
                      << SDL_GetError() << std::endl;
            return false;
        }
        m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
        if (m_renderer)
        {
            std::cout << "Renderer Created!" << std::endl;
            m_isRunning = true;
        }
        else
        {
            std::cout << "Renderer Creation Failed>\n"
                      << SDL_GetError() << std::endl;
            return false;
        }
    }
    else
    {
        std::cout << "SDL_Init Error\n"
                  << SDL_GetError() << std::endl;
        return false;
    }
    return true;
}
void Window::setFullScreen()
{
    m_flags = m_flags | SDL_WINDOW_FULLSCREEN;
}
void Window::setRendererColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha)
{
    SDL_SetRenderDrawColor(m_renderer, red, green, blue, alpha);
}
