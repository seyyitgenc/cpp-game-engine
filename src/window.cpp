#include <headers/window.h>

Window::Window()
{
}

Window::~Window()
{
}

bool Window::Init(const char *title, int x, int y, int w, int h, int flag)
{
    if (m_fullscreen)
        flag = flag | SDL_WINDOW_FULLSCREEN;
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
    {
        std::cout << "Subsystems Initialized!" << std::endl;
        m_window = SDL_CreateWindow(title, x, y, w, h, flag);
        if (m_window != NULL)
        {
            std::cout << "Window Created!" << std::endl;
            SDL_SetWindowMinimumSize(m_window, 200, 200);
            m_screenSurface = SDL_GetWindowSurface(m_window);
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
            std::cout << "Rendere Created!" << std::endl;
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
    m_red = red;
    m_blue = blue;
    m_green = green;
    m_alpha = alpha;
}
void Window::Render()
{
    SDL_SetRenderDrawColor(m_renderer, m_red, m_green, m_blue, m_alpha);
    SDL_RenderClear(m_renderer);
    SDL_RenderPresent(m_renderer);
}
SDL_Surface *Window::loadSurface(std::string path)
{
    SDL_Surface *opitimizedSurface = nullptr;
    SDL_Surface *loadedSurface = SDL_LoadBMP(path.c_str());

    if (loadedSurface == NULL)
        std::cout << "Unable to load image\n"
                  << SDL_GetError() << std::endl;
    else
    {
        opitimizedSurface = SDL_ConvertSurface(loadedSurface, m_screenSurface->format, 0);
        if (opitimizedSurface == NULL)
            std::cout << "Unable to optimize image\n"
                      << SDL_GetError() << std::endl;
        SDL_FreeSurface(loadedSurface);
    }
    return opitimizedSurface;
}
bool Window::loadMedia()
{
    // load scretching surface
    m_stretchedSurface = loadSurface("stretch.bmp");
    if (getScreenSurface() == NULL)
    {
        std::cout << "Failed to load stcretching image" << std::endl;
        return false;
    }
    return true;
}
void Window::close(){
    SDL_FreeSurface(m_stretchedSurface);
    m_stretchedSurface=NULL;
}