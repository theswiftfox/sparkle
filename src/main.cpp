#include "Application.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main(const int argc, char** argv)
{
	std::cout << "Waiting for GPU Debug injection. Press \"Enter\" to continue!" << std::endl;
	std::cin.ignore();
	auto& app = Sparkle::App::getHandle();

	try {
		std::string config = "assets/settings.ini";
		if (argc > 1) {
			config = argv[1];
		}
		app.run(config);
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl
		          << "Press any key to exit..." << std::endl;
		std::cin.ignore();
		return EXIT_FAILURE;
	}
	// std::cin.ignore();
	return EXIT_SUCCESS;
}