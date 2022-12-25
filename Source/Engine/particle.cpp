#include "particle.h"

Particle::Particle(int x, int y)
{
    m_posX = x - 5 + (rand() % 25);
    m_posY = y - 5 + (rand() % 25);
    m_frame = rand() % 5;
    m_particle.x = m_posX;
    m_particle.y = m_posY;
    m_particle.w = 5 + rand() % 5;
    m_particle.h = 5 + rand() % 5;
}

Particle::~Particle()
{
    // TODO : free particle array
}

bool Particle::isDead()
{
    return m_frame > 10;
}

void Particle::render()
{
    SDL_SetRenderDrawColor(gRenderer, 100, 255, 100, 255);
    SDL_RenderFillRectF(gRenderer, &m_particle);
    SDL_RenderDrawRectF(gRenderer, &m_particle);
    m_frame++;
}