import mars;

#include <SDL3/SDL.h>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    constexpr float sensitivity = 1500.0f;
    constexpr float sensitivityY = sensitivity / 2.0f;
    float dx, dy;
    SDL_GetRelativeMouseState(&dx, &dy);
    if(dx != 0.0f) {
        //horizontal rotation
        float angleX = dx / sensitivity * -1.0f;
        float angleY = dy / sensitivity * -1.0f;
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angleX, game.camera.up);
        glm::vec4 newDir = rotation * glm::vec4(game.camera.dir, 0.0f);
        //vertical rotation
        if((game.camera.dir.y < 1.0f and angleY < 0) or (game.camera.dir.y >= -1.0f and angleY > 0)) {
            glm::vec3 const axis = glm::cross(glm::vec3(newDir.x, newDir.y, newDir.z), game.camera.up);
            rotation = glm::rotate(glm::mat4(1.0f), angleY, axis);
        }
        newDir = rotation * newDir;
        game.camera.dir = glm::vec3(newDir.x, newDir.y, newDir.z);
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
