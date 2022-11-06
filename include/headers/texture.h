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
    // must define this functions from derivied classes
    Texture();                                                                   // constructor
    ~Texture();                                                                  // destructor
    virtual SDL_Texture *getTexture() = 0;                                       // Get texture
    virtual void setTextureHeight(int height) = 0;                               // Set texture Height
    virtual void setTextureWidth(int width) = 0;                                 // Set texture Width
    virtual int getTextureHeight() = 0;                                          // Get texture Height
    virtual int getTextureWidth() = 0;                                           // Get texture Width
    virtual void freeTexture() = 0;                                              // free used texture
    virtual void Draw(int x, int y, SDL_Rect *clip, SDL_Renderer *renderer) = 0; // draw texture

protected:
    SDL_Texture *m_texture;
    int m_width = 64, m_height = 64;
};

// Texture derivied Sprite class
class Sprite : Texture
{
public:
    Sprite();
    ~Sprite();
    SDL_Texture *getTexture() { return m_texture; }

    void setTextureHeight(int height) { m_height = height; } // set texture height
    void setTextureWidth(int width) { m_width = width; }     // set texture width
    int getTextureHeight() { return m_height; }              // get texture height
    int getTextureWidth() { return m_width; }                // get texture width

    void setColorMode(Uint8 red, Uint8 green, Uint8 blue) { SDL_SetTextureColorMod(m_texture, red, green, blue); } // set texture color mode
    void setAlphaMode(Uint8 alpha) { SDL_SetTextureAlphaMod(m_texture, alpha); }                                   // set texture alpha mode
    void setBlendMode(SDL_BlendMode blending) { SDL_SetTextureBlendMode(m_texture, blending); }                    // set texture blend mode

    bool initTexture(std::string path, SDL_Renderer *renderer);      // initialize the sprite texture from path
    void Draw(int x, int y, SDL_Rect *clip, SDL_Renderer *renderer); // draws sprite
    void freeTexture();                                              // free texture used by object
};
// Texture derivied TextTexture Class
// This class handle text to texture conversion
class TextTexture : Texture
{
public:
    TextTexture();
    ~TextTexture();
    SDL_Texture *getTexture() { return m_texture; }

    void setTextureHeight(int height) { m_height = height; }                                                        // set texture height
    void setTextureWidth(int width) { m_width = width; }                                                            // set texture width
    int getTextureHeight() { return m_height; }                                                                     // get texture height
    int getTextureWidth() { return m_width; }                                                                       // get texture width
    void setTextColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha) { m_textColor = {red, green, blue, alpha}; } // set text color and alpha
    SDL_Color getTextColor() { return m_textColor; }
    bool loadRenderedText(std::string textTexture, SDL_Color textColor, SDL_Renderer *renderer); // load text into the m_texture object
    void Draw(int x, int y, SDL_Rect *clip, SDL_Renderer *renderer);                             // draw m_texture into the screen
    bool loadFont(std::string path);
    void closeFont();   // close font used by object
    void freeTexture(); // free texture used by object

private:
    TTF_Font *m_font = NULL;
    std::string m_path = "fonts/aerial.ttf";
    SDL_Color m_textColor = {255, 255, 255, 255};
};