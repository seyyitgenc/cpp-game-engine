#ifndef SHAPE_H
#define SHAPE_H
#include <SDL2/SDL.h>
#include <headers/vector2d.h>
#endif
class Player
{
public:
    Vector2D size = {64, 64};
    Vector2D pos = {0, 0};
    Vector2D vel = {0, 0};
    SDL_Texture *texture;
    Player();
    ~Player();
    void update(float dt)
    {
    }
    void setSize(Vector2D size) { this->size = size; }
    void setPosition(Vector2D pos) { this->pos = pos; }
    Vector2D &getSize() { return this->size; }
    Vector2D &getPositin() { return this->pos; }
    void SetTexture(SDL_Texture *texture) { this->texture = texture; }
    void draw(int x, int y, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip, SDL_Renderer *renderer);

private:
};