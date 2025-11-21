#include "mars.h"

#include <stdio.h>

int main(int argc, char** argv) {
    MarsGame g;
    MarsError result = marsInit(&g, NULL);
    if(result.key != MARS_ALL_OKAY) {
	printf("%s\n", result.message);
	return 1;
    }
    marsQuit(&g);
    return 0;
}
