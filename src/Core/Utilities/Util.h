#pragma once

#include <sstream>

namespace Sparkle {
class Logger {
public:
    static void toStdout(std::string msg, std::string func = "");
};

class IO {
public:
	static bool isImageExtension(std::string ext);
};
}

#define LOGADDRSTDOUT(msg, ptr)                       \
    {                                                 \
        auto fname = std::string(__func__);           \
        std::stringstream strm;                       \
        strm << msg << " (" << ptr << ")";            \
        Sparkle::Logger::toStdout(strm.str(), fname); \
    }

#define LOGSTDOUT(msg)                         \
    {                                          \
        auto fname = std::string(__func__);    \
        Sparkle::Logger::toStdout(msg, fname); \
    }
