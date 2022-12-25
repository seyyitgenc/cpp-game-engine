#include "texture.h"

class Particle
{
public:
    Particle(int x, int y);
    void render();
    bool isDead();
    ~Particle();

private:
    int m_posX, m_posY;
    int m_frame;
    SDL_FRect m_particle;
};
