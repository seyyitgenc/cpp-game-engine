#pragma once
#include "ECS/AssetManager.h"
#include "SDL.h"
#include <array>
class UILabel : public Component
{
public:
    UILabel(int xpos, int ypos, std::string text, std::string font) : m_labelText(text), m_labelFont(font)
    {
        m_labelRect.x = xpos;
        m_labelRect.y = ypos;
        setLabelText(m_labelText);
        m_textColor = {0, 255, 0, 255}; // default text color
    }
    UILabel(int xpos, int ypos, std::string text, std::string font, SDL_Color &color) : m_labelText(text), m_labelFont(font), m_textColor(color)
    {
        m_labelRect.x = xpos;
        m_labelRect.y = ypos;
        setLabelText(m_labelText);
    }
    ~UILabel() = default;
    void setLabelText(const std::string &labelText)
    {
        // TODO must optimize this section
        // NOTE : TTF_RenderText_Blended is loading text into to high res texture.
        SDL_Surface *surface = TTF_RenderText_Blended(AssetManager::get().getFont(m_labelFont), labelText.c_str(), m_textColor);
        SDL_DestroyTexture(m_labelTexture); // prevents memory leak
        m_labelTexture = SDL_CreateTextureFromSurface(Engine::get().getRenderer(), surface);
        SDL_FreeSurface(surface);
        SDL_QueryTexture(m_labelTexture, nullptr, nullptr, &m_labelRect.w, &m_labelRect.h);
    }

    void draw() override final
    {
        SDL_RenderCopy(Engine::get().getRenderer(), m_labelTexture, nullptr, &m_labelRect);
    }

private:
    SDL_Rect m_labelRect;
    std::string m_labelText;
    std::string m_labelFont;
    SDL_Color m_textColor;
    SDL_Texture *m_labelTexture = nullptr;
};