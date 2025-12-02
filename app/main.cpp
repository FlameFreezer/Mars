import mars;

#include <iostream>

#define MARS_REPORT(proc) procResult = proc;\
if(!procResult.okay()) {\
    std::cout << procResult.getMessage() << '\n';\
    return 1;\
}

int main(int argc, char** argv) {
    mars::Error<mars::noreturn> procResult;
    MARS_REPORT(mars::init());
    MARS_REPORT(mars::run());
    return mars::quit();
}
