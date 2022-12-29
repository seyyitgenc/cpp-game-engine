#include "particle.h"

Particle::Particle(int x, int y)
    : m_posX(x - 5 + (rand() % 25)), m_posY(y - 5 + (rand() % 25)), m_frame(rand() % 5), m_particle(4, 60) {
    m_particle.setPos(m_posX, m_posY);
    m_particle.setColor({(Uint8)rand() % 255, (Uint8)rand() % 255, 255, 255});
}

Particle::~Particle() {}

bool Particle::isDead() { return m_frame > 10; }

void Particle::render() {
    // SDL_SetRenderDrawColor(gRenderer, 100, 255, 100, 255);
    m_particle.filledCircle();
    // SDL_RenderFillRectF(gRenderer, &m_particle);
    // SDL_RenderDrawRectF(gRenderer, &m_particle);
    m_frame++;
}