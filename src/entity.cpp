#include <headers/entity.h>
Player::Player()
{
}

Player::~Player()
{
}
// draw Texture object into the screen
void Player::draw(int x, int y, SDL_Rect *clip, double angle, SDL_Point *center, SDL_RendererFlip flip, SDL_Renderer *renderer)
{
    SDL_Rect renderQuad = {x, y, 120, 120};
    if (clip != NULL)
    {
        renderQuad.w = clip->w;
        renderQuad.h = clip->h;
    }
    SDL_RenderCopyEx(renderer, texture, clip, &renderQuad, angle, center, flip);
}