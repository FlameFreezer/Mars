#ifndef MARS_H
#include "mars.hpp"
#endif

#include <iostream>

int main(int argc, char** argv) {
    MarsGame g;
    MarsError result = marsInit(g, nullptr);
    if(result.key != MARS_ALL_OKAY) {
        std::cout << result.message << '\n';
        return 1;
    }
    marsQuit(g);
    return 0;
}
