#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <tinygltf/tiny_gltf.h>

namespace Sparkle {
namespace Import {
	class glTFLoader {
	public:
		void loadFromFile(std::string filePath);

	private:
	};
} // namespace Utils
} // namespace Sparkle

#endif