#ifndef WINDOW_H
#define WINDOW_H
#include <SDL2/SDL.h>
#include <iostream>
#endif

class Window
{
public:
    Window();
    ~Window();                                                              // destruct everthing used by windows event when everthing is done
    void Draw(SDL_Texture *texture, SDL_Rect r1);              // draw into the screen
    void setFullScreen();                                                   // Enables fullscreen
    void setRendererColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha); // Set renderer color
    bool Init(const char *title, int x, int y, int flag);                   // initialize the window
    bool isOpen() { return m_isRunning; }                                   // returns isRunning
    void setisOpen(bool isRunning) { m_isRunning = isRunning; }             // Set isRunning
    SDL_Window *getWindow() { return m_window; }                            // return window
    SDL_Renderer *getRenderer() { return m_renderer; }                      // return renderer
    SDL_Surface *getScreenSurface() { return m_screenSurface; }             // returns screenSurface
    SDL_Surface *getStretchedSurface() { return m_stretchedSurface; }       // returns stretchedSurface
    SDL_Surface *loadSurface(std::string path);                             // this function is used with getStretchedSurface function
    void setScreenWidth(int width) { m_screenWidth = width; }               // Set screen width
    void setScreenHeight(int height) { m_screenHeight = height; }           // Set screen height
    int getScreenWidth() { return m_screenWidth; }                          // Get screen width
    int getScreenHeight() { return m_screenHeight; }                        // Get Screen height
    bool loadMedia();

public:
private:
    int m_screenWidth = 800, m_screenHeight = 600;
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SDL_Surface *m_screenSurface;
    SDL_Surface *m_stretchedSurface;
    int m_flags = 0;
    Uint8 m_red = 10, m_green = 10, m_blue = 10, m_alpha = 0;
    bool m_fullscreen = false;
    bool m_isRunning = false;
};
