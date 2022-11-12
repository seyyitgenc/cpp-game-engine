// sdl
#ifndef sdl_h
#define sdl_h
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#endif
// texture
#ifndef texture_h
#define texture_h
#include <headers/texture.h>
#endif
// vector
#ifndef vector_h
#define vector_h
#include <headers/vector2d.h>
#endif
#include <iostream>
#include <vector>
class Window
{
public:
    Window();
    ~Window(); // destruct everthing used by windows event when everthing is done

    void setRendererColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha); // Set renderer color
    bool Init(const char *title, int x, int y, int flag);                   // initialize the window
    bool isOpen() { return m_isRunning; }                                   // returns isRunning
    void setisOpen(bool isRunning) { m_isRunning = isRunning; }             // Set isRunning
    SDL_Window *getWindow() { return m_window; }                            // return window
    SDL_Renderer *getRenderer() { return m_renderer; }                      // return renderer
    SDL_Surface *getSurface() { return m_surface; }                         // Get surface
public:
    void setFullScreen();                                         // Enables fullscreen
    void setScreenWidth(int width) { m_screenWidth = width; }     // Set screen width
    void setScreenHeight(int height) { m_screenHeight = height; } // Set screen height
    int getScreenWidth() { return m_screenWidth; }                // Get screen width
    int getScreenHeight() { return m_screenHeight; }              // Get Screen height
    bool loadMedia();

public:
    // draw functions

    // FIXME errors can occur due to the wrong object pass
    // this template will work with Sprite,TextToTexture please ensure you pass correct object
    // You can give specific position, size, clip, angle, center, flip values to draw object into the screenkd
    template <typename T>
    void Draw(T &object, vi2d pos, vi2d size, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip)
    {
        SDL_Rect dstRect = {pos.x, pos.y, size.x, size.y};
        if (clip != NULL)
        {
            dstRect.w = clip->w;
            dstRect.h = clip->h;
        }
        SDL_RenderCopyEx(m_renderer, object.getTexture(), clip, &dstRect, angle, center, flip);
    }

    // FIXME errors can occur due to the wrong object pass
    // this template will work with Sprite,TextToTexture please ensure you pass correct object
    // You can give specific position and size to draw into the screen
    template <typename T>
    void Draw(T &object, vi2d pos, vi2d size)
    {
        SDL_Rect dstRect = {pos.x, pos.y, size.x, size.y};
        SDL_RenderCopy(m_renderer, object.getTexture(), NULL, &dstRect);
    }

    // FIXME errors can occur due to the wrong object pass
    // this template will work with Sprite,TextToTexture please ensure you pass correct object
    // You can simply pass object to draw
    template <typename T>
    void Draw(T &object)
    {
        vi2d pos = object.getPos();
        vi2d size = object.getSize();
        SDL_Rect dstRect = {pos.x, pos.y, size.x, size.y};
        SDL_RenderCopy(m_renderer, object.getTexture(), NULL, &dstRect);
    }

private:
    int m_screenWidth = 1024, m_screenHeight = 768;
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SDL_Surface *m_surface;
    int m_flags = 0;
    Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
    bool m_fullscreen = false;
    bool m_isRunning = false;
    
};
