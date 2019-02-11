#pragma once

#include <sstream>

namespace Engine {
    class Logger {
    public:
        static void toStdout(std::string msg, std::string func = "");
    };
}


#define LOGADDRSTDOUT(msg, ptr)                     \
{                                                   \
    auto fname = std::string(__func__);             \
    std::stringstream strm;                         \
    strm << msg << " (" << ptr << ")";              \
    Engine::Logger::toStdout(strm.str(), fname);    \
}

#define LOTSTDOUT(msg)				        \
{											\
	auto fname = std::string(__func__);	    \
	Engine::Logger::toStdout(msg, fname);   \
}

