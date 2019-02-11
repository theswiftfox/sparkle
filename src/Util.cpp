#include "Util.h"

#include <iostream>

void Engine::Logger::toStdout(std::string msg, std::string func) {
    {
            std::cout << msg;
            if (func.size() != 0) {
                std::cout << " in " << func;
            }
            std::cout << std::endl;
        }
}