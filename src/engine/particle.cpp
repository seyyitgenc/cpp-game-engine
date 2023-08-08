// #include "particle.h"

// Particle::Particle(float x, float y)
//     : m_posX(x - 5 + (rand() % 25)), m_posY(y - 5 + (rand() % 25)), m_frame(0), m_particle(6, 30) {
//     m_particle.setPos(m_posX, m_posY);
//     m_particle.setColor({(Uint8)(rand() % 255), (Uint8)(rand() % 255), 255, 255});
// }

// Particle::~Particle() {}

// bool Particle::isDead() { return m_frame > 6; }

// void Particle::render() {
//     m_frame++;
//     m_particle.filledCircle();
// }