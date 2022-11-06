#ifndef TEXTURE_H
#define TEXTURE_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#endif

// Texture interface
class Texture
{
public:
    // must override this functions from derivied classes                                                          // destructor
    Texture(); // constructor
    ~Texture();
    virtual SDL_Texture *getTexture() = 0;                                  // Get texture
    virtual void setTextureHeight(int height) = 0;                          // Set texture Height
    virtual void setTextureWidth(int width) = 0;                            // Set texture Width
    virtual int getTextureHeight() = 0;                                     // Get texture Height
    virtual int getTextureWidth() = 0;                                      // Get texture Width
    virtual bool initTexture(std::string path, SDL_Renderer *renderer) = 0; // initialize texture
    virtual void freeTexture() = 0;                                         // free used texture
    virtual void setColor(Uint8 red, Uint8 green, Uint8 blue) = 0;          // set texture color mode
    virtual void setAlpha(Uint8 alpha) = 0;                                 // Set texture alpha mode
    virtual void setBlendMode(SDL_BlendMode blending) = 0;                  // set texture blend mode

protected:
    SDL_Texture *m_texture;
    int m_width = 64, m_height = 64;
};

// Texture derivied sprite class
class Sprite : Texture
{
public:
    Sprite();
    ~Sprite();
    SDL_Texture *getTexture() { return m_texture; }
    void setTextureHeight(int height) { m_height = height; }
    void setTextureWidth(int width) { m_width = width; }
    int getTextureHeight() { return m_height; }
    int getTextureWidth() { return m_width; }
    void setColor(Uint8 red, Uint8 green, Uint8 blue) { SDL_SetTextureColorMod(m_texture, red, green, blue); }
    void setAlpha(Uint8 alpha) { SDL_SetTextureAlphaMod(m_texture, alpha); }
    void setBlendMode(SDL_BlendMode blending) { SDL_SetTextureBlendMode(m_texture, blending); }
    void freeTexture();
    bool initTexture(std::string path, SDL_Renderer *renderer) override;
};
// texture derivied TextTexture Class
// this class will handle text to texture conversion events
class TextTexture : Texture
{
public:
    TextTexture();
    ~TextTexture();
    SDL_Texture *getTexture() { return m_texture; }

    bool initTexture(std::string path, SDL_Renderer *renderer) { return true; } // initialize texture

    void setTextureHeight(int height) { m_height = height; }
    void setTextureWidth(int width) { m_width = width; }
    int getTextureHeight() { return m_height; }
    int getTextureWidth() { return m_width; }
    void setColor(Uint8 red, Uint8 green, Uint8 blue) { SDL_SetTextureColorMod(m_texture, red, green, blue); }
    void setAlpha(Uint8 alpha) { SDL_SetTextureAlphaMod(m_texture, alpha); }
    void setBlendMode(SDL_BlendMode blending) { SDL_SetTextureBlendMode(m_texture, blending); }
    bool loadRenderedText(std::string textTexture, SDL_Color textColor, SDL_Renderer *renderer);
    void freeTexture();
    void closeFont();

private:
    TTF_Font *m_font;
    SDL_Color m_textColor = {0, 0, 0, 0};
};