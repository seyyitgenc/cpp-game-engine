#include "Engine/Engine.h"
#include "ECS/AssetManager.h"

#include "ECS/Components/Sprite.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/CollisionBox.h"
#include "Engine/Collider.h"

#include <limits>
#include <algorithm>

Engine *Engine::s_instance;

// TODO : move cpp files to it's related paths
Engine::Engine()
{
    m_isRunning = false;
    m_window = nullptr;
    m_renderer = nullptr;
}
Engine::~Engine() {}

struct rect
{
    vf2d pos;
    vf2d size;
    vf2d vel;
};

// initialize window and renderer
void Engine::initApp()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
        std::cerr << SDL_GetError() << std::endl;
    auto wflags = (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow("Game Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, wflags);
    if (!m_window)
        std::cout << "Window creation error!" << std::endl;
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
        std::cout << "Renderer creation error!" << std::endl;
    m_clearColor = DARK;
    m_isRunning = true;

    fpsTimer.start();
}

Entity *p1;
Entity *p2;

std::vector<rect> vrects;
// initialize objects that will be used by game
void Engine::initEntities()
{
    manager = new Manager();
    // TODO : Tile map can be done here
    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 28);

    AssetManager::get().loadTexture("enemy", "assets/texture.png");
    AssetManager::get().loadTexture("player", "assets/tile.png");

    vrects.push_back({{0, 0}, {64, 64}});
    vrects.push_back({{300, 300}, {128, 100}});
    // player
    // p1 = &manager->addEntity();
    // p1->addComponent<Sprite>(m_renderer, "player", 64, 64);
    // p1->addComponent<RigidBody>(); // for physics calculation
    // p1->addComponent<CollisionBox>(m_renderer, 64, 64);
    // p2 = &manager->addEntity();
    // p2->addComponent<Sprite>(m_renderer, "enemy", 64, 64);
    // p2->addComponent<CollisionBox>(m_renderer, 100, 100);
    // p2->getComponent<Transform>().position = {300, 300};

    // p1->addComponent<Collider>();
}

bool RayVsRect(const vf2d &ray_origin, const vf2d &ray_dir, const rect *target, vf2d &contact_point, vf2d &contact_normal, float &t_hit_near)
{
    contact_normal = {0, 0};
    contact_point = {0, 0};

    // Calculate intersections with rectangle bounding axes
    vf2d t_near = (target->pos - ray_origin) / ray_dir;
    vf2d t_far = (target->pos + target->size - ray_origin) / ray_dir;

    if (std::isnan(t_far.y) || std::isnan(t_far.x))
        return false;
    if (std::isnan(t_near.y) || std::isnan(t_near.x))
        return false;

    // Sort distances
    if (t_near.x > t_far.x)
        std::swap(t_near.x, t_far.x);
    if (t_near.y > t_far.y)
        std::swap(t_near.y, t_far.y);

    // Early rejection
    if (t_near.x > t_far.y || t_near.y > t_far.x)
        return false;

    // Closest 'time' will be the first contact
    t_hit_near = std::max(t_near.x, t_near.y);

    // Furthest 'time' is contact on opposite side of target
    float t_hit_far = std::min(t_far.x, t_far.y);

    // Reject if ray direction is pointing away from object
    if (t_hit_far < 0)
        return false;

    // Contact point of collision from parametric line equation
    contact_point = ray_origin + ray_dir * t_hit_near;

    if (t_near.x > t_near.y)
        if (ray_dir.x < 0)
            contact_normal = {1, 0};
        else
            contact_normal = {-1, 0};
    else if (t_near.x < t_near.y)
        if (ray_dir.y < 0)
            contact_normal = {0, 1};
        else
            contact_normal = {0, -1};

    // Note if t_near == t_far, collision is principly in a diagonal
    // so pointless to resolve. By returning a CN={0,0} even though its
    // considered a hit, the resolver wont change anything.
    return true;
}

bool DynamicRectVsRect(const rect *r_dynamic, const float fTimeStep, const rect &r_static,
                       vf2d &contact_point, vf2d &contact_normal, float &contact_time)
{
    // Check if dynamic rectangle is actually moving - we assume rectangles are NOT in collision to start
    if (r_dynamic->vel.x == 0 && r_dynamic->vel.y == 0)
        return false;

    // Expand target rectangle by source dimensions
    rect expanded_target;
    expanded_target.pos = r_static.pos - r_dynamic->size / 2;
    expanded_target.size = r_static.size + r_dynamic->size;

    if (RayVsRect(r_dynamic->pos + r_dynamic->size / 2, r_dynamic->vel * fTimeStep, &expanded_target, contact_point, contact_normal, contact_time))
        return (contact_time >= 0.0f && contact_time < 1.0f);
    else
        return false;
}

bool ResolveDynamicRectVsRect(rect *r_dynamic, const float fTimeStep, rect *r_static)
{
    vf2d contact_point, contact_normal;
    float contact_time = 0.0f;
    if (DynamicRectVsRect(r_dynamic, fTimeStep, *r_static, contact_point, contact_normal, contact_time))
    {
        r_dynamic->vel += contact_normal * vf2d(std::abs(r_dynamic->vel.x), std::abs(r_dynamic->vel.y)) * (1 - contact_time);
        return true;
    }
    return false;
}

void Engine::events()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0)
    {
        if (event.type == SDL_QUIT)
            m_isRunning = false;
        if (event.type == SDL_WINDOWEVENT_RESIZED)
        {
        }
        else if (event.type == SDL_KEYDOWN)
        {
        }
    }
}

void Engine::update(float &dt)
{
    vrects[0].vel = {0, 0};
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_W])
    {
        vrects[0].vel.y -= 0.5f;
    }
    if (state[SDL_SCANCODE_S])
    {
        vrects[0].vel.y += 0.5f;
    }
    if (state[SDL_SCANCODE_D])
    {
        vrects[0].vel.x += 0.5f;
    }
    if (state[SDL_SCANCODE_A])
    {
        vrects[0].vel.x -= 0.5f;
    }
    vf2d cp, cn;
    float t = 0;
    std::vector<std::pair<int, float>> z;
    for (size_t i = 1; i < vrects.size(); i++)
    {
        if (DynamicRectVsRect(&vrects[0], dt, vrects[i], cp, cn, t))
        {
            z.push_back({i, t});
        }
    }
    std::sort(z.begin(), z.end(), [](const std::pair<int, float> &a, const std::pair<int, float> &b)
              { return a.second < b.second; });

    // Now resolve the collision in correct order
    for (auto j : z)
        ResolveDynamicRectVsRect(&vrects[0], dt, &vrects[j.first]);

    vrects[0].pos += vrects[0].vel * dt;
    manager->update(dt);
}

void Engine::render()
{
    static int countedFrames = 0;
    SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
    SDL_RenderClear(m_renderer);
    SDL_SetRenderDrawColor(m_renderer, 255, 30, 30, 255);

    for (int i = 0; i < vrects.size(); i++)
    {
        SDL_FRect rect = {
            vrects[i].pos.x,
            vrects[i].pos.y,
            vrects[i].size.x,
            vrects[i].size.y,
        };
        SDL_RenderDrawRectF(m_renderer, &rect);
    }

    manager->draw();
    SDL_RenderPresent(m_renderer);
    countedFrames++;
}

void Engine::clean()
{
    AssetManager::get().clean();
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
    TTF_Quit();
    IMG_Quit();
}

void Engine::quit()
{
    m_isRunning = false;
}
