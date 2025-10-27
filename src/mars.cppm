module;

#include <iostream>

export module mars;
export namespace mars {
	void sayHello() {
		std::cout << "Hello, world!" << std::endl;
	}
}
