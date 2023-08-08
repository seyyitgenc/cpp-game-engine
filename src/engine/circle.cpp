// #include "circle.h"

// Circle::~Circle() {}

// void Circle::hollowCircle() {
//     float new_x = 0;
//     float new_y = 0;
//     float old_x = m_X;
//     float old_y = m_Y;
//     float step = (M_PI * 2) / m_step;
//     SDL_SetRenderDrawColor(gRenderer, m_color.r, m_color.g, m_color.b, m_color.a);

//     for (float theta = 0; theta <= (M_PI * 2); theta += step) {
//         new_x = m_X + (m_radius * cos(theta));
//         new_y = m_Y - (m_radius * sin(theta));
//         SDL_RenderDrawLine(gRenderer, old_x, old_y, new_x, new_y);

//         old_x = new_x;
//         old_y = new_y;
//     }

//     new_x = m_X + (m_radius * cos(0));
//     new_y = m_Y - (m_radius * sin(0));
//     SDL_RenderDrawLine(gRenderer, old_x, old_y, new_x, new_y);
// }
// void Circle::filledCircle() {
//     float new_x = 0;
//     float new_y = 0;
//     float old_x = m_X;
//     float old_y = m_Y;
//     float step = (M_PI * 2) / m_step;

//     std::vector<SDL_Vertex> vert = {
//         {
//             SDL_FPoint{m_X, m_Y},
//             SDL_Color{m_color.r, m_color.g, m_color.b, m_color.a},
//             SDL_FPoint{0},
//         },
//         {
//             SDL_FPoint{new_x, new_y},
//             SDL_Color{m_color.r, m_color.g, m_color.b, m_color.a},
//             SDL_FPoint{0},
//         },
//         {
//             SDL_FPoint{old_x, old_y},
//             SDL_Color{m_color.r, m_color.g, m_color.b, m_color.a},
//             SDL_FPoint{0},
//         },
//     };
//     SDL_SetRenderDrawColor(gRenderer, m_color.r, m_color.g, m_color.b, m_color.a);

//     for (float theta = 0; theta <= (M_PI * 2); theta += step) {
//         new_x = m_X + (m_radius * cos(theta));
//         new_y = m_Y - (m_radius * sin(theta));

//         vert[1].position = {new_x, new_y};
//         vert[2].position = {old_x, old_y};
//         SDL_RenderDrawLine(gRenderer, old_x, old_y, new_x, new_y);
//         SDL_RenderGeometry(gRenderer, nullptr, vert.data(), vert.size(), nullptr, 0);
//         old_x = new_x;
//         old_y = new_y;
//     }

//     new_x = m_X + (m_radius * cos(0));
//     new_y = m_Y - (m_radius * sin(0));

//     vert[1].position = {new_x, new_y};
//     vert[2].position = {old_x, old_y};

//     SDL_RenderDrawLine(gRenderer, old_x, old_y, new_x, new_y);
//     SDL_RenderGeometry(gRenderer, nullptr, vert.data(), vert.size(), nullptr, 0);
// }

// void Circle::setPos(const float& x, const float& y) {
//     m_X = x;
//     m_Y = y;
// }

// vf2d Circle::getPos() { return vf2d(m_X, m_Y); }

// void Circle::setRadius(const float& radius) { m_radius = radius; }

// float Circle::getRadius() { return m_radius; }

// void Circle::setColor(const SDL_Color& color) { m_color = color; }

// SDL_Color Circle::getColor() { return m_color; }