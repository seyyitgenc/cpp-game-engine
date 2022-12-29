#include "engine.h"
// Components
#include <random>

#include "ECS/assetmanager.h"
#include "circle.h"
#include "collider.h"
#include "particle.h"

Engine* Engine::s_instance;

Engine::Engine() { m_isRunning = false; }
Engine::~Engine() { this->clean(); }

std::vector<Entity*> colRects;

Entity* frametimeLabel;
Entity* fpsLabel;
Entity* cameraPos;
Entity* playerPos;

Collider collisionResolver;

Clock fpsTimer;

std::vector<std::unique_ptr<Particle>> particles;
std::vector<std::unique_ptr<Circle>> circles;

// initialize objects that will be used by game
void Engine::initEntities() {
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

    // UI Elements
    frametimeLabel = &gEntityManager->addEntity();
    frametimeLabel->addComponent<UILabel>(0, 0, "", "sans");
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
    colRects[0]->addComponent<RigidBody>();  // for physics calculation
    colRects[0]->addComponent<CollisionBox>(gRenderer, 64, 128);

    // enemy
    colRects.push_back(&gEntityManager->addEntity());
    colRects[1]->addComponent<Transform>(0, 0);
    colRects[1]->addComponent<Sprite>(gRenderer, "enemy", 128, 128);
    colRects[1]->addComponent<CollisionBox>(gRenderer, 10, 100);
    colRects[1]->getComponent<Transform>().position = {900, 600};
    for (int i = 0; i < 100; i++) {
        Circle* circle{new Circle(20, 30)};
        circle->setPos(400 + rand() % 1200, rand() % 900);
        // circle->setColor({255, (Uint8)rand() % 255, (Uint8)rand() % 255, 255});
        std::unique_ptr<Circle> uniquePtr{circle};
        circles.emplace_back(std::move(uniquePtr));
    }
    for (int i = 0; i < 12; i++) {
        Particle* particle = {new Particle(0, 0)};
        std::unique_ptr<Particle> uniquePtr{particle};
        particles.emplace_back(std::move(uniquePtr));
    }
    fpsTimer.start();
}
// main game loop
void Engine::run() {
    m_isRunning = initGlobals();
    this->initEntities();
    // Mix_PlayMusic(gMusic, -1);
    fpsTimer.start();
    while (this->isRunning()) {
        // limitFrameRate();
        this->events();
        this->update(getTimeStep());
        this->render();
    }
}
void renderParticles() {
    int x, y;
    SDL_GetMouseState(&x, &y);
    // * if particle/particles isDead() flag that and erase them from container
    // * erasing them from container will not cause problem because we used smart pointer
    // note: this auto represents this ---> std::vector<std::unique_ptr<Particle>>::iterator
    auto object = find_if(particles.begin(), particles.end(), [&](std::unique_ptr<Particle>& obj) { return obj->isDead(); });
    particles.erase(std::remove(particles.begin(), particles.end(), *object));

    // create new particle here
    std::unique_ptr<Particle> uniquePtr{new Particle(x, y)};
    particles.emplace_back(std::move(uniquePtr));
    // draw particles here
    std::cout << particles.size() << std::endl;
    for (auto& item : particles) {
        item->render();
    }
}
void Engine::events() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) m_isRunning = false;
        if (event.type == SDL_WINDOWEVENT_RESIZED) {
        } else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) m_isRunning = false;
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
            }
        }
    }
}

void Engine::update(const float& dt) {
    collisionResolver.resolveSweptAABB(colRects, dt);
    gGroundTiles->update(dt);
    gEntityManager->update(dt);
    gUIManager->update(dt);
}
int x, y;
Circle test(200, 50);
void Engine::render() {
    static int countedFrames = 0;
    countedFrames++;
    SDL_GetMouseState(&x, &y);
    // TODO :  do this section on different file named UI
    frametimeLabel->getComponent<UILabel>().setLabelText("frame time : " + std::to_string(getDeltaTime()));

    fpsLabel->getComponent<UILabel>().setLabelText("FPS : " + std::to_string(getFrameRate(countedFrames, fpsTimer.getTicks())));

    cameraPos->getComponent<UILabel>().setLabelText("Cam Pos : " + std::to_string(Camera::get().getPos().x) + " , " +
                                                    std::to_string(Camera::get().getPos().y));

    playerPos->getComponent<UILabel>().setLabelText(
        "Player Pos : " + std::to_string(colRects[0]->getComponent<Transform>().position.x) + " , " +
        std::to_string(colRects[0]->getComponent<Transform>().position.y));

    // camera pos TODO : this camera can be done on globals.h
    Camera::get().setPos(colRects[0]->getComponent<Transform>().position.x, colRects[0]->getComponent<Transform>().position.y);

    SDL_SetRenderDrawColor(gRenderer, rendererColor.r, rendererColor.g, rendererColor.b, rendererColor.a);

    SDL_RenderClear(gRenderer);  // clear screen
    gGroundTiles->draw();        // draw tilemap
    gEntityManager->draw();      // draw all entities
    gUIManager->draw();          // draw UI Elements

    // test.setPos(x, y);
    // test.filledCircle();
    // for (auto& items : circles) {
    //     items->filledCircle();
    // }

    SDL_SetRenderDrawColor(gRenderer, 0, 255, 0, 255);
    // camera color
    SDL_RenderDrawRectF(gRenderer,
                        &Camera::get().getCameraRect());  // camera rectangle
    renderParticles();
    SDL_RenderPresent(gRenderer);
}

void Engine::clean() {
    // TODO destroy colrects entities
    AssetManager::get().clean();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    Mix_Quit();
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

void Engine::quit() { m_isRunning = false; }