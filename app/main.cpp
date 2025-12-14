import mars;

#include <SDL3/SDL.h>

using ErrorNoreturn = mars::Error<mars::noreturn>;

ErrorNoreturn run() noexcept {
    mars::Game g;
    if(!g.getProcResult().okay()) return g.getProcResult();
    bool shouldKeepRunning = true;
    while(shouldKeepRunning) {
    	SDL_Event e;
    	while(SDL_PollEvent(&e) and shouldKeepRunning) {
    	    switch(e.type) {
    	    case SDL_EVENT_QUIT:
                shouldKeepRunning = false;
                break;
            default:
                break;
    	    }
    	}
        g.draw();
        if(!g.getProcResult().okay()) return g.getProcResult();
    }
    return mars::success();
}

int main(int argc, char** argv) {
    if(!mars::init().report()) return 1;
    if(!run().report()) return 1;
    mars::quit();
    return 0;
}
