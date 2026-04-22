#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Bird {
    glm::vec2 pos;
    glm::vec2 vel;
    float radius;
};

struct Pipe {
    float x;
    float gapCenterY;
    float gapHalfSize;
    float width;
};

class Game {
public:
    Bird bird;
    std::vector<Pipe> pipes;
    float timeSinceLastPipe;
    float pipeSpawnInterval;
    float gravity;
    float flapImpulse;
    float worldWidth;
    float worldHeight;

    Game();
    void update(float dt);
    void flap();
    void spawnPipe();
    bool checkCollision() const;
};
