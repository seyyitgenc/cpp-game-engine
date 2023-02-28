#pragma once
#include "circle.h"
#include "clock.h"
#include "texture.h"

// TODO tih particle class can be done with templates
class Particle {
public:
    Particle(float x, float y);
    void render();
    bool isDead();
    ~Particle();

private:
    float m_posX, m_posY;
    int m_frame;
    Circle m_particle;
    // TODO this 30 value can be stored as fps value
    float m_lifeTime = 1000.0f / 30.0f;
};
