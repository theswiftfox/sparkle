#include "Application.h"

#include <iostream>
#include <stdexcept>

int main(const int argc, char** argv) {
	auto& app = Engine::App::getHandle();
	const auto validation = true;

	try {
		std::string config = "assets/settings.ini";
		if (argc > 1) {
			config = argv[1];
		}
		app.run(config, validation);
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl << "Press any key to exit..." << std::endl;
		std::cin.ignore();
		return EXIT_FAILURE;
	}
	std::cin.ignore();
	return EXIT_SUCCESS;
}