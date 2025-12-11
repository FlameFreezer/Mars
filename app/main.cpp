import mars;

#include <SDL3/SDL.h>

bool run() noexcept {
    mars::Game g;
    if(!g.getProcResult().report()) return false;
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
    }
    return true;
}

int main(int argc, char** argv) {
    if(!mars::init().report()) return 1;
    if(!run()) return 1;
    mars::quit();
    return 0;
}
