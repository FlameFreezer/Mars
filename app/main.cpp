#ifndef MARS_H
#include "mars.hpp"
#endif

#include <iostream>

#define MARS_REPORT(proc) procResult = proc;\
if(!procResult.okay()) {\
    std::cout << procResult.getMessage() << '\n';\
    return 1;\
}

void mainLoop(mars::Error<mars::noreturn>& procResult);

int main(int argc, char** argv) {
    mars::Error<mars::noreturn> procResult;
    MARS_REPORT(mars::init());
    mainLoop(procResult);
    return mars::quit();
}

void mainLoop(mars::Error<mars::noreturn>& procResult) {
    mars::Game g(procResult);
    if(!procResult.okay()) {
        std::cout << procResult.getMessage() << '\n';
        return;
    }

}
