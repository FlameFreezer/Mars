#include "mars.h"

int main(int argc, char** argv) {
    MarsGame g;
    marsInit(&g, NULL);
    marsQuit(&g);
    return 0;
}
