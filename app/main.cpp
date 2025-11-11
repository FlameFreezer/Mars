import mars;

#include <iostream>
#include <exception>

int main() {
    int exitCode = 0;
    try {
	mars::Game game("Mars Test");
	game.Run();
    }
    catch(std::exception e) {
	std::cerr << e.what() << std::endl;
	exitCode = 1;
    }
    return exitCode;
}
