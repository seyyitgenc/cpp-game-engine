#pragma once
#include <SDL2/SDL.h>
#include "globals.h"
#include "vector2d.h"

class Circle {
public:
    /// @brief
    /// @param radius determines the radius of a circle
    /// @param step   determines the line count. (line and triangle count for filled circle)
    Circle() : m_radius(10), m_step(50){};  // default constructor for circle
    Circle(float radius, int step) : m_radius(radius), m_X(0), m_Y(0), m_step(step), m_color({255, 255, 255, 255}){};
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
    int m_step;
};
