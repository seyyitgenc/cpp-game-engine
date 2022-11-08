#ifndef WINDOW_H
#define WINDOW_H
#include <SDL2/SDL.h>
#include <iostream>
#include <headers/texture.h>
#include <headers/vector2d.h>
#endif

class Window
{
public:
    Window();
    ~Window(); // destruct everthing used by windows event when everthing is done

    void setFullScreen();                                                   // Enables fullscreen
    void setRendererColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha); // Set renderer color
    bool Init(const char *title, int x, int y, int flag);                   // initialize the window
    bool isOpen() { return m_isRunning; }                                   // returns isRunning
    void setisOpen(bool isRunning) { m_isRunning = isRunning; }             // Set isRunning
    SDL_Window *getWindow() { return m_window; }                            // return window
    SDL_Renderer *getRenderer() { return m_renderer; }                      // return renderer
    void setScreenWidth(int width) { m_screenWidth = width; }               // Set screen width
    void setScreenHeight(int height) { m_screenHeight = height; }           // Set screen height
    int getScreenWidth() { return m_screenWidth; }                          // Get screen width
    int getScreenHeight() { return m_screenHeight; }                        // Get Screen height
    bool loadMedia();

public:
    // draw functions
    template <typename T>
    void Draw(T &object, vi2d pos, vi2d size, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip, SDL_Renderer *renderer)
    {
        SDL_Rect dstRect = {pos.x, pos.y, size.x, size.y};
        if (clip != NULL)
        {
            dstRect.w = clip->w;
            dstRect.h = clip->h;
        }
        SDL_RenderCopyEx(renderer, object.getTexture(), clip, &dstRect, angle, center, flip);
    }
    // use template here
    void Draw(Sprite &object, vi2d pos, vi2d size)
    {
    }
    template <typename T>
    void Draw(T &object)
    {
        // get it's height
        // get it's location
    }

private:
    int m_screenWidth = 800, m_screenHeight = 600;
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    int m_flags = 0;
    Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
    bool m_fullscreen = false;
    bool m_isRunning = false;
};
