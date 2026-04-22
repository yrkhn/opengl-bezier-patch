#include "game.h"
#include <cstdlib>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>

Game::Game() {
    worldWidth = 960.0f;
    worldHeight = 540.0f;
    bird.pos = glm::vec2(worldWidth * 0.25f, worldHeight * 0.5f);
    bird.vel = glm::vec2(0.0f, 0.0f);
    bird.radius = 18.0f;
    timeSinceLastPipe = 0.0f;
    pipeSpawnInterval = 1.2f;
    gravity = -1800.0f;
    flapImpulse = 520.0f;
}

void Game::update(float dt) {
    bird.vel.y += gravity * dt;
    bird.pos += bird.vel * dt;
    timeSinceLastPipe += dt;
    if (timeSinceLastPipe >= pipeSpawnInterval) {
        spawnPipe();
        timeSinceLastPipe = 0.0f;
    }
    for (auto& p : pipes) p.x -= 220.0f * dt;
    while (!pipes.empty() && pipes.front().x < -300.0f) pipes.erase(pipes.begin());
}
void Game::flap() { bird.vel.y = flapImpulse; }
void Game::spawnPipe() {
    Pipe p;
    p.x = worldWidth + 100.0f;
    p.gapCenterY = 140.0f + (std::rand() % 260);
    p.gapHalfSize = 80.0f;
    p.width = 80.0f;
    pipes.push_back(p);
}

static bool circleIntersectsRect(const glm::vec2& c, float r, float left, float bottom, float right, float top) {
    float closestX = glm::clamp(c.x, left, right);
    float closestY = glm::clamp(c.y, bottom, top);
    float dx = c.x - closestX;
    float dy = c.y - closestY;
    return (dx * dx + dy * dy) < (r * r);
}
bool Game::checkCollision() const {
    if (bird.pos.y - bird.radius < 0.0f) return true;
    if (bird.pos.y + bird.radius > worldHeight) return true;
    for (const auto& p : pipes) {
        float left = p.x - p.width * 0.5f;
        float right = p.x + p.width * 0.5f;
        float topGap = p.gapCenterY + p.gapHalfSize;
        float bottomGap = p.gapCenterY - p.gapHalfSize;
        if (circleIntersectsRect(bird.pos, bird.radius, left, topGap, right, worldHeight)) return true;
        if (circleIntersectsRect(bird.pos, bird.radius, left, 0.0f, right, bottomGap)) return true;
    }
    return false;
}
