import mars;

#include <SDL3/SDL.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "mars_macros.h"

using ErrorNoreturn = mars::Error<mars::noreturn>;

constexpr float speed = 1.0f;

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
    if(game.keyState[SDL_SCANCODE_D])
        game.camera.pos.x += speed * game.getDeltaTimeSeconds().count();
    if(game.keyState[SDL_SCANCODE_A])
        game.camera.pos.x -= speed * game.getDeltaTimeSeconds().count();
    if(game.keyState[SDL_SCANCODE_S])
        game.camera.pos.z -= speed * game.getDeltaTimeSeconds().count();
    if(game.keyState[SDL_SCANCODE_W])
        game.camera.pos.z += speed * game.getDeltaTimeSeconds().count();
    if(game.keyState[SDL_SCANCODE_SPACE]) 
        game.camera.pos.y -= speed * game.getDeltaTimeSeconds().count();
    if(game.keyState[SDL_SCANCODE_LSHIFT])
        game.camera.pos.y += speed * game.getDeltaTimeSeconds().count();
    if(game.keyState[SDL_SCANCODE_ESCAPE])
        game.setFlags(mars::flagBits::stopExecution);
}

void handleMouseInput(mars::Game& game) noexcept {
    constexpr float sensitivity = 1000.0f;
    float dx, dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    if(dx != 0.0f or dy != 0.0f) {
        float const angleX = dx / sensitivity;
        float const angleY = dy / sensitivity;
        constexpr glm::vec3 start(0.0f, 0.0f, 1.0f);
        float const sqrMag = angleX * angleX + angleY * angleY;
        glm::vec3 const end(angleX, angleY, std::sqrt(1.0f - sqrMag));
        glm::quat const drag(glm::dot(start, end), glm::cross(start, end));
        glm::quat const now = drag * glm::quat(0.0f, game.camera.dir);
        game.camera.dir = glm::vec3(now.x, now.y, now.z);
    }
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
