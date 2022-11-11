// sdl
#ifndef sdl_h
#define sdl_h
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif
// vector
#ifndef vector_h
#define vector_h
#include <headers/vector2d.h>
#endif
#include <iostream>
// TODO: REFACTOR TEXTURE CLASS COMPLETLY THIS STRUCTURE SEEMS WRONGGGGGG!!!!!
// TODO : set texture can be initialized with width and height variable seperatly  like setSize(width,heigh)
// Texture interface
class Texture
{
public:
    // must define this functions from derivied classes
    virtual SDL_Texture *getTexture() = 0;  // Get texture
    virtual int getHeight() = 0;            // Get texture Height
    virtual int getWidth() = 0;             // Get texture Width
    virtual void setSize(vi2d size) = 0;    // Set Texture Size
    virtual void setPos(vi2d pos) = 0;      // Get Texture size
    virtual void setSize(int w, int h) = 0; // Set Size
    virtual void setPos(int x, int y) = 0;  // Set Pos
    virtual vi2d getSize() = 0;             // Get texture size
    virtual vi2d getPos() = 0;              // Get texture pos

protected:
    void freeTexture(); // free used texture
    SDL_Texture *m_texture = NULL;
    int m_width = 64, m_height = 64;
    vi2d m_size = {100, 150}; // size vector
    vi2d m_pos = {0, 0};      // pos vector
};

// Texture derivied Sprite class
class Sprite : Texture
{
public:
    Sprite();
    ~Sprite();

public:
    int getHeight() { return m_size.y; }             // get texture height
    int getWidth() { return m_size.x; }              // get texture width
    void setSize(vi2d size) { m_size = size; }       // Set texture size
    void setPos(vi2d pos) { m_pos = pos; }           // Set texture pos
    vi2d getSize() { return m_size; }                // Get Texture size
    vi2d getPos() { return m_pos; }                  // Get texture pos
    void setSize(int w, int h) { m_size = {w, h}; }; // Set Size
    void setPos(int x, int y) { m_pos = {x, y}; };   // Set Pos

public:
    // Sprite texture functions
    SDL_Texture *getTexture() { return m_texture; }
    void setColorMode(Uint8 red, Uint8 green, Uint8 blue) { SDL_SetTextureColorMod(m_texture, red, green, blue); } // set texture color mode
    void setAlphaMode(Uint8 alpha) { SDL_SetTextureAlphaMod(m_texture, alpha); }                                   // set texture alpha mode
    void setBlendMode(SDL_BlendMode blending) { SDL_SetTextureBlendMode(m_texture, blending); }                    // set texture blend mode
    SDL_Surface *loadSurface(std::string path, SDL_Surface *screenSurface);
    bool initTexture(std::string path, SDL_Renderer *renderer); // initialize the sprite texture from path
    void freeTexture() { Texture::freeTexture(); }              // free texture used by sprite class
};
// Texture derivied TextToTexture Class
// This class handle text to texture conversion
class TextToTexture : Texture
{
public:
    TextToTexture();  // constructor
    ~TextToTexture(); // destructor

public:
    int getHeight() { return m_size.y; }             // get texture height
    int getWidth() { return m_size.x; }              // get texture width
    void setSize(vi2d size) { m_size = size; }       // Set texture size
    void setPos(vi2d pos) { m_pos = pos; }           // Set texture pos
    vi2d getSize() { return m_size; }                // Get Texture size
    vi2d getPos() { return m_pos; }                  // Get texture pos
    void setSize(int w, int h) { m_size = {w, h}; }; // Set Size
    void setPos(int x, int y) { m_pos = {x, y}; };   // Set Pos

public:
    //  TextToTexture texture  functions
    static bool setFont(const std::string path);                                                                    // load font 
    SDL_Texture *getTexture() { return m_texture; }                                                                 // Get texture
    void setTextColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) { m_textColor = {red, green, blue, alpha}; } // set text color and alpha
    SDL_Color getTextColor() { return m_textColor; }                                                                // get text color
    bool loadRenderedText(std::string TextToTexture, SDL_Color textColor, SDL_Renderer *renderer);                  // load text into the m_texture object                                                          // Get texture pos
    static void closeFont();                                                                                               // close font used by object
    void freeTexture() { Texture::freeTexture(); }                                                                  // free texture used by object

private:
    static TTF_Font *m_font;
    static std::string m_path;
    SDL_Color m_textColor = {255, 255, 255, 255};
};