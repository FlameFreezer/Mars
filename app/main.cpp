import mars;

#include <SDL3/SDL.h>

#include "mars_macros.h"

using ErrorNoreturn = mars::Error<mars::noreturn>;

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

ErrorNoreturn mainLoop(mars::Game& g) noexcept {
    while(!g.hasFlags(mars::flagBits::stopExecution)) {
    	SDL_Event e;
    	while(SDL_PollEvent(&e)) {
            handleEvent(g, e);
            if(g.hasFlags(mars::flagBits::stopExecution)) return mars::success();
    	}
        TRY(g.draw());
    }
    return mars::success();
}

int main(int argc, char** argv) {
    mars::Game g;
    if(!g.init().report()) return 1;
    if(!mainLoop(g).report()) return 1;
    return 0;
}
