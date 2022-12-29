#pragma once
#include "SDL.h"
#include "globals.h"
#include "vector2d.h"
class Circle {
public:
    /// @brief
    /// @param radius determines the radius of a circle
    /// @param step   determines the line count. (line and triangle count for filled circle)
    Circle(float radius, float step) : m_radius(radius), m_X(0), m_Y(0), m_step(step), m_color({255, 255, 255, 255}){};
    ~Circle();

    // getters and setters
    void setPos(const float& x, const float& y);
    vf2d getPos();
    void setRadius(const float& radius);
    float getRadius();
    void setColor(const SDL_Color& color);
    SDL_Color getColor();

    // circle drawing algorithms
    void hollowCircle();
    void filledCircle();

private:
    SDL_Color m_color;
    float m_X, m_Y;
    float m_radius;
    float m_step;
};
