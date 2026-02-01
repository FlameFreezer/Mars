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

mars::ID cube;

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
    if(game.keyState[SDL_SCANCODE_ESCAPE]) {
        game.setFlags(mars::flagBits::stopExecution);
    }

    if(velocity != glm::vec3(0.0f)) {
        velocity = glm::normalize(velocity) * SPEED * game.getDeltaTimeSeconds();
        game.camera.pos += velocity;
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

void fixCube(mars::Game& game) {
    float halfd = glm::tan(game.camera.fov / 2.0f);
    mars::Rect2D const windowDimensions = game.getWindowDimensions();
    if(windowDimensions.h < windowDimensions.w) halfd *= (static_cast<float>(windowDimensions.w) / windowDimensions.h);
    game.objects.at(game.objects.positions, cube) = glm::vec3(-halfd, -halfd, 0.0f);
}

ErrorNoreturn mainLoop(mars::Game& game) noexcept {
    initCamera(game);
    auto cubemesh = game.loadMesh("CUBE");
    if(!cubemesh) return cubemesh.moveError();

    auto texture = game.loadTexture(std::string(MARS_ASSETS_PATH) + "S_Placeholder.png");
    if(!texture) return texture.moveError();

    float halfd = glm::tan(game.camera.fov / 2.0f);
    mars::Rect2D windowDimensions = game.getWindowDimensions();
    if(windowDimensions.h < windowDimensions.w) halfd *= (static_cast<float>(windowDimensions.w) / windowDimensions.h);
    cube = game.createObject(cubemesh, texture, glm::vec3(-halfd, -halfd, 0.0f), glm::vec3(2.0f * halfd));

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
        fixCube(game);
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
