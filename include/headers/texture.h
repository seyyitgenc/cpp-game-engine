#ifndef TEXTURE_H
#define TEXTURE_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#endif
// TODO : BUILD BETTER DRAW FUNCTIONS
// TODO : make virtual void Draw()=0 for SDL_RenderCopyEx() function
// Texture interface
class Texture
{
public:
    Texture();  // Constructor
    ~Texture(); // Destructor
public:
    // must define this functions from derivied classes
    virtual SDL_Texture *getTexture() = 0;      // Get texture
    virtual int getHeight() = 0;                // Get texture Height
    virtual int getWidth() = 0;                 // Get texture Width
    virtual void setTextureSize(vi2d size) = 0; // Set Texture Size
    virtual void setTexturePos(vi2d pos) = 0;   // Get Texture size
    virtual vi2d getTextureSize() = 0;          // Get texture size
    virtual vi2d getTexturePos() = 0;           // Get texture pos

protected:
    void freeTexture(); // free used texture
    SDL_Texture *m_texture = NULL;
    vi2d m_size = {m_width, m_height}; // size vector
    int m_width = 64, m_height = 64;
    vi2d m_pos = {0, 0}; // pos vector
};

// Texture derivied Sprite class
class Sprite : Texture
{
public:
    Sprite();
    ~Sprite();

public:
    int getHeight() { return m_height; }              // get texture height
    int getWidth() { return m_width; }                // get texture width
    void setTextureSize(vi2d size) { m_size = size; } // Set texture size
    void setTexturePos(vi2d pos) { m_pos = pos; }     // Set texture pos
    vi2d getTextureSize() { return m_size; }          // Get Texture size
    vi2d getTexturePos() { return m_pos; }            // Get texture pos

public:
    // Sprite texture functions
    SDL_Texture *getTexture() { return m_texture; }
    void setColorMode(Uint8 red, Uint8 green, Uint8 blue) { SDL_SetTextureColorMod(m_texture, red, green, blue); } // set texture color mode
    void setAlphaMode(Uint8 alpha) { SDL_SetTextureAlphaMod(m_texture, alpha); }                                   // set texture alpha mode
    void setBlendMode(SDL_BlendMode blending) { SDL_SetTextureBlendMode(m_texture, blending); }                    // set texture blend mode
    bool initTexture(std::string path, SDL_Renderer *renderer);                                                    // initialize the sprite texture from path
    void freeTexture() { Texture::freeTexture(); }                                                                 // free texture used by sprite class
};
// Texture derivied TextToTexture Class
// This class handle text to texture conversion
class TextToTexture : Texture
{
public:
    TextToTexture();
    ~TextToTexture();
    int getHeight() { return m_height; }              // get texture height
    int getWidth() { return m_width; }                // get texture width
    void setTextureSize(vi2d size) { m_size = size; } // Set texture size
    void setTexturePos(vi2d pos) { m_pos = pos; }     // Set texture pos
    vi2d getTextureSize() { return m_size; }          // Get Texture size
    vi2d getTexturePos() { return m_pos; }            // Get texture pos

public:
    // TextToTexture texture  functions
    SDL_Texture *getTexture() { return m_texture; }
    void setTextColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) { m_textColor = {red, green, blue, alpha}; } // set text color and alpha
    SDL_Color getTextColor() { return m_textColor; }                                                                // get text color
    bool loadRenderedText(std::string TextToTexture, SDL_Color textColor, SDL_Renderer *renderer);                  // load text into the m_texture object                                                          // Get texture pos
    bool loadFont(std::string path);                                                                                // load font
    void closeFont();                                                                                               // close font used by object
    void freeTexture() { Texture::freeTexture(); }                                                                  // free texture used by object

private:
    TTF_Font *m_font = NULL;
    std::string m_path = "fonts/aerial.ttf";
    SDL_Color m_textColor = {255, 255, 255, 255};
};