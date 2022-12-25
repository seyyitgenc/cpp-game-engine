#include "engine.h"
// Components
#include "collider.h"
#include "particle.h"
#include "ECS/assetmanager.h"
#include <random>

Engine *Engine::s_instance;

Engine::Engine()
{
    m_isRunning = false;
}
Engine::~Engine()
{
    this->clean();
}

std::vector<Entity *> colRects;

// Entity *frametimeLabel;
Entity *fpsLabel;
Entity *cameraPos;
Entity *playerPos;

Collider collisionResolver;

Clock fpsTimer;
Particle *particles[12];

// initialize objects that will be used by game
void Engine::initEntities()
{
    // TODO : add tilemap collision
    // TODO : add tilemap array to use later on for A* pathfinding algorithm

    // load fonts
    AssetManager::get().loadFont("aerial", "fonts/aerial.ttf", 16);
    AssetManager::get().loadFont("oswald", "fonts/oswald.ttf", 16);
    AssetManager::get().loadFont("sans", "fonts/sans.ttf", 16);

    // load textures
    AssetManager::get().loadTexture("test", "assets/texture.png");
    AssetManager::get().loadTexture("tile", "assets/tile.png");
    AssetManager::get().loadTexture("player", "assets/player.png");
    AssetManager::get().loadTexture("enemy", "assets/enemy.png");
    AssetManager::get().loadTexture("playerset", "assets/player-set.png");

    // for (int i = 0; i < 4832; i++)
    // {
    //     static float x = 0, y = 0;
    //     auto &entity = gEntityManager->addEntity();
    //     entity.addComponent<Transform>(0, 0);
    //     entity.addComponent<Sprite>(gRenderer, "tile", 16, 16);
    //     entity.getComponent<Transform>().position = vf2d{x, y};
    //     x += 16;
    //     if (x > LEVEL_WIDTH)
    //     {
    //         x = 0;
    //         y += 16;
    //     }
    // }

    // UI Elements
    // frametimeLabel = &gEntityManager->addEntity();
    // frametimeLabel->addComponent<UILabel>(0, 0, "", "sans");
    fpsLabel = &gUIManager->addEntity();
    fpsLabel->addComponent<UILabel>(0, 30, "", "sans");
    cameraPos = &gUIManager->addEntity();
    cameraPos->addComponent<UILabel>(0, 60, "", "sans");
    playerPos = &gUIManager->addEntity();
    playerPos->addComponent<UILabel>(0, 90, "", "sans");

    // player
    colRects.push_back(&gEntityManager->addEntity());
    colRects[0]->addComponent<Transform>(0, 0);
    colRects[0]->addComponent<Sprite>(gRenderer, "player", 64, 128);
    colRects[0]->addComponent<RigidBody>(); // for physics calculation
    colRects[0]->addComponent<CollisionBox>(gRenderer, 64, 128);

    // enemy
    colRects.push_back(&gEntityManager->addEntity());
    colRects[1]->addComponent<Transform>(0, 0);
    colRects[1]->addComponent<Sprite>(gRenderer, "enemy", 128, 128);
    colRects[1]->addComponent<CollisionBox>(gRenderer, 128, 128);
    colRects[1]->getComponent<Transform>().position = {900, 600};
    for (int i = 0; i < 12; i++)
    {
        particles[i] = new Particle(0, 0);
    }
    fpsTimer.start();
}
// main game loop
void Engine::run()
{
    m_isRunning = initGlobals();
    this->initEntities();
    fpsTimer.start();
    // Mix_PlayMusic(gMusic, -1);

    while (this->isRunning())
    {
        // limitFrameRate();
        this->events();
        this->update(getTimeStep());
        this->render();
    }
}
void renderParticles()
{
    int x, y;
    SDL_GetMouseState(&x, &y);
    for (int i = 0; i < 12; i++)
    {
        if (particles[i]->isDead())
        {
            delete particles[i];
            particles[i] = new Particle(x, y);
        }
    }
    for (int i = 0; i < 12; i++)
    {
        particles[i]->render();
    }
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
            if (event.key.keysym.sym == SDLK_ESCAPE)
                m_isRunning = false;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            if (event.button.button == SDL_BUTTON_LEFT)
            {
            }
        }
    }
}

void Engine::update(float &dt)
{
    collisionResolver.resolveSweptAABB(colRects, dt);
    gGroundTiles->update(dt);
    gEntityManager->update(dt);
    gUIManager->update(dt);
}

void Engine::render()
{
    static int countedFrames = 0;
    countedFrames++;
    // TODO :  do this section on different file named UI
    // frametimeLabel->getComponent<UILabel>().setLabelText("frame time : " + std::to_string(getTimeStep()));
    fpsLabel->getComponent<UILabel>().setLabelText("FPS : " + std::to_string(getFrameRate(countedFrames, fpsTimer.getTicks())));
    cameraPos->getComponent<UILabel>().setLabelText("Cam Pos : " + std::to_string(Camera::get().getPos().x) + " , " + std::to_string(Camera::get().getPos().y));
    playerPos->getComponent<UILabel>().setLabelText("Player Pos : " + std::to_string(colRects[0]->getComponent<Transform>().position.x) + " , " + std::to_string(colRects[0]->getComponent<Transform>().position.y));

    // camera pos TODO : this camera can be done on globals.h
    Camera::get().setPos(colRects[0]->getComponent<Transform>().position.x, colRects[0]->getComponent<Transform>().position.y);

    SDL_SetRenderDrawColor(gRenderer, rendererColor.r, rendererColor.g, rendererColor.b, rendererColor.a);

    SDL_RenderClear(gRenderer); // clear screen
    gGroundTiles->draw();       // draw tilemap
    gEntityManager->draw();     // draw all entities
    gUIManager->draw();         // draw UI Elements

    // SDL_SetRenderDrawColor(gRenderer, 0, 255, 0, 255);              // camera color
    // SDL_RenderDrawRectF(gRenderer, &Camera::get().getCameraRect()); // camera rectangle
    renderParticles();
    SDL_RenderPresent(gRenderer);
}

void Engine::clean()
{
    // TODO destroy colrects entities
    AssetManager::get().clean();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    Mix_Quit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void Engine::quit()
{
    m_isRunning = false;
}