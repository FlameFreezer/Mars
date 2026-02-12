import mars;

#include <string>

#include <SDL3/SDL.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mars_macros.h"

using ErrorNoreturn = mars::Error<mars::noreturn>;

#define SPEED 1.0f
#define PLAYER_SPEED 200.0f

mars::ID player;

void handleEvent(mars::Game& game, SDL_Event const& e) noexcept {
    switch(e.type) {
    case SDL_EVENT_QUIT: [[fallthrough]];
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        game.setFlags(mars::flagBits::stopExecution);
        break;
    case SDL_EVENT_WINDOW_RESIZED:
        game.setFlags(mars::flagBits::recreateSwapchain);
        break;
    default:
        break;
    }
}

void handleKeyboardInput(mars::Game& game) noexcept {
    game.updateKeyState();
    glm::vec3 velocity(0.0f);
    glm::vec3 playerVelocity(0.0f);
    if(game.keyState[SDL_SCANCODE_D]) {
        glm::vec3 const right(glm::normalize(glm::cross(game.camera.dir, game.camera.up)));
        velocity += right;
    }
    if(game.keyState[SDL_SCANCODE_A]) {
        glm::vec3 const left(glm::normalize(glm::cross(game.camera.up, game.camera.dir)));
        velocity += left;
    }
    if(game.keyState[SDL_SCANCODE_S]) {
        glm::vec3 dir(game.camera.dir);
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        velocity -= dir;
    }
    if(game.keyState[SDL_SCANCODE_W]) {
        glm::vec3 dir(game.camera.dir);
        dir.y = 0.0f;
        dir = glm::normalize(dir);
        velocity += dir;
    }
    if(game.keyState[SDL_SCANCODE_SPACE]) {
        velocity += game.camera.up;
    }
    if(game.keyState[SDL_SCANCODE_LSHIFT]) {
        velocity -= game.camera.up;
    }
    if(game.keyState[SDL_SCANCODE_RIGHT]) {
        playerVelocity.x += 1.0f;
    }
    if(game.keyState[SDL_SCANCODE_LEFT]) {
        playerVelocity.x -= 1.0f;
    }
    if(game.keyState[SDL_SCANCODE_DOWN]) {
        playerVelocity.y += 1.0f;
    }
    if(game.keyState[SDL_SCANCODE_UP]) {
        playerVelocity.y -= 1.0f;
    }
    if(game.keyState[SDL_SCANCODE_ESCAPE]) {
        game.setFlags(mars::flagBits::stopExecution);
    }

    if(velocity != glm::vec3(0.0f)) {
        velocity = glm::normalize(velocity) * SPEED * game.getDeltaTimeSeconds();
        game.camera.pos += velocity;
    }
    if(playerVelocity != glm::vec3(0.0f)) {
        playerVelocity = glm::normalize(playerVelocity) * PLAYER_SPEED * game.getDeltaTimeSeconds();
        game.objects.at(game.objects.positions, player) += playerVelocity;
    }
}

void handleMouseInput(mars::Game& game) noexcept {
    float dx, dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    if(dx == 0.0f and dy == 0.0f) return;
    game.camera.rotate(dx, dy);
}

void handleGamepad(mars::Game& game) noexcept {
}

void initCamera(mars::Game& game) noexcept {
    game.camera.pos = glm::vec3(0.0f, 0.0f, -1.0f);
    game.camera.dir = glm::vec3(0.0f, 0.0f, 1.0f);
    game.camera.up = glm::vec3(0.0f, -1.0f, 0.0f);
    game.camera.fov = glm::radians(45.0f);
    game.camera.aspect = mars::Camera::autoAspect;
}

ErrorNoreturn mainLoop(mars::Game& game) noexcept {
    initCamera(game);
    mars::ID texture = game.loadTexture(std::string(MARS_ASSETS_PATH) + "S_Placeholder.png");
    player = game.createObject(game.shapes.square, texture, glm::vec3(0.0f), glm::vec3(100.0f));
    while(!game.hasFlags(mars::flagBits::stopExecution)) {
    	SDL_Event e;
        game.updateTime();
    	while(SDL_PollEvent(&e)) {
            handleEvent(game, e);
            if(game.hasFlags(mars::flagBits::stopExecution)) return mars::success();
    	}
        handleKeyboardInput(game);
        handleMouseInput(game);
        handleGamepad(game);
        TRY(game.draw());
    }
    return mars::success();
}

int main(int argc, char** argv) {
    mars::Game g;
    if(!g.init().report()) return 1;
    if(!mainLoop(g).report()) return 1;
    return 0;
}
