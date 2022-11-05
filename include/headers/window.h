#ifndef WINDOW_H
#define WINDOW_H
#include <SDL2/SDL.h>
#include <iostream>
#endif

class Window
{
public:
    Window();
    ~Window();
    void Draw(SDL_Texture* texture);
    void setFullScreen();
    void setRendererColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha);
    bool Init(const char *title, int x, int y, int w, int h, int flag);
    bool isOpen() { return m_isRunning; }
    void setisOpen(bool isRunning) { m_isRunning = isRunning; }
    SDL_Window *getWindow() { return m_window; }
    SDL_Renderer *getRenderer() { return m_renderer; }
    SDL_Surface *getScreenSurface() { return m_screenSurface; }
    SDL_Surface *getStretchedSurface() { return m_stretchedSurface; }
    SDL_Surface *loadSurface(std::string path);
    bool loadMedia();
    void close();

public:
private:
    int SCREEN_WIDTH = 1366, SCREEN_HEIGHT = 768;
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SDL_Surface *m_screenSurface;
    SDL_Surface *m_stretchedSurface;
    int m_flags = 0;
    Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
    bool m_fullscreen = false;
    bool m_isRunning = false;
};
