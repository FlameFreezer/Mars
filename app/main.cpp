import mars;

#include <SDL3/SDL.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mars_macros.h"

using ErrorNoreturn = mars::Error<mars::noreturn>;

constexpr float speed = 2.0f;

void handleEvent(mars::Game& game, SDL_Event const& e) noexcept {
    switch(e.type) {
    case SDL_EVENT_QUIT: [[fallthrough]];
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        game.setFlags(mars::flagBits::stopExecution);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        game.setRendererFlags(mars::flagBits::recreateSwapchain);
        break;
    default:
        break;
    }
}

void handleKeyboardInput(mars::Game& game) noexcept {
    game.updateKeyState();
    if(game.keyState[SDL_SCANCODE_D]) {
        glm::vec3 const right = glm::normalize(glm::cross(game.camera.dir, game.camera.up));
        game.camera.pos += right * speed * game.getDeltaTimeSeconds();
    }
    if(game.keyState[SDL_SCANCODE_A]) {
        glm::vec3 const left = glm::normalize(glm::cross(game.camera.up, game.camera.dir));
        game.camera.pos += left * speed * game.getDeltaTimeSeconds();
    }
    if(game.keyState[SDL_SCANCODE_S]) {
        glm::vec3 dir(game.camera.dir);
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        game.camera.pos -= dir * speed * game.getDeltaTimeSeconds();
    }
    if(game.keyState[SDL_SCANCODE_W]) {
        glm::vec3 dir(game.camera.dir);
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        game.camera.pos += dir * speed * game.getDeltaTimeSeconds();
    }
    if(game.keyState[SDL_SCANCODE_SPACE]) {
        game.camera.pos += game.camera.up * speed * game.getDeltaTimeSeconds();
    }
    if(game.keyState[SDL_SCANCODE_LSHIFT]) {
        game.camera.pos -= game.camera.up * speed * game.getDeltaTimeSeconds();
    }
    if(game.keyState[SDL_SCANCODE_ESCAPE])
        game.setFlags(mars::flagBits::stopExecution);
}

void handleMouseInput(mars::Game& game) noexcept {
    #define SENSITIVITY 0.001f
    #define MAX_Y 0.95f
    float dx, dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    if(dx == 0.0f and dy == 0.0f) return;

    if(game.camera.dir.y >= MAX_Y and dy > 0.0f) dy = 0.0f;
    else if(game.camera.dir.y <= -MAX_Y and dy < 0.0f) dy = 0.0f;

    float const angleX = -dx * SENSITIVITY;
    float const angleY = -dy * SENSITIVITY;
    constexpr glm::mat4 i(1.0f);
    glm::vec4 dir;

    dir = glm::rotate(i, angleX, game.camera.up) * glm::vec4(game.camera.dir, 0.0f);
    game.camera.dir = {dir.x, dir.y, dir.z};

    glm::vec3 const axis = glm::cross(game.camera.dir, game.camera.up);
    dir = glm::rotate(i, angleY, axis) * glm::vec4(game.camera.dir, 0.0f);
    game.camera.dir = {dir.x, dir.y, dir.z};
}

ErrorNoreturn mainLoop(mars::Game& game) noexcept {
    while(!game.hasFlags(mars::flagBits::stopExecution)) {
    	SDL_Event e;
        game.updateTime();
    	while(SDL_PollEvent(&e)) {
            handleEvent(game, e);
            if(game.hasFlags(mars::flagBits::stopExecution)) return mars::success();
    	}
        handleKeyboardInput(game);
        handleMouseInput(game);
        TRY(game.draw());
    }
    return mars::success();
}

int main(int argc, char** argv) {
    mars::Game g;
    if(!g.init().report()) return 1;
    g.camera.pos = glm::vec3(0.0f, 0.0f, -2.0f);
    g.camera.dir = glm::vec3(0.0f, 0.0f, 1.0f);
    g.camera.up = glm::vec3(0.0f, -1.0f, 0.0f);
    g.camera.fov = glm::radians(45.0f);
    g.camera.aspect = mars::Camera::autoAspect;
    if(!mainLoop(g).report()) return 1;
    return 0;
}
