#include "Util.h"

#include <iostream>

void Sparkle::Logger::toStdout(std::string msg, std::string func)
{
    {
        std::cout << msg;
        if (func.size() != 0) {
            std::cout << " in " << func;
        }
        std::cout << std::endl;
    }
}

bool Sparkle::IO::isImageExtension(std::string ext)
{
	if (ext.compare(".png") == 0)
		return true;
	if (ext.compare(".jpg") == 0)
		return true;
	if (ext.compare(".jpeg") == 0)
		return true;
	if (ext.compare(".bmp") == 0)
		return true;
	return false;
}