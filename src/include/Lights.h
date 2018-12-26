#pragma once

#include <glm/glm.hpp>

namespace Engine {
	namespace Lights{
		enum LType {
			SPARKLE_LIGHT_TYPE_UNDEFINED = 0x0,
			SPARKLE_LIGHT_TYPE_DIRECTIONAL = 0x1,
			SPARKLE_LIGHT_TYPE_POINT = 0x2
		};

		class Light {
		public:
			Light(LType type, glm::vec3 loc = glm::vec3(0.0f), glm::vec3 col = glm::vec3(0.0f));

			auto getType() const { return mType; }
		private:
			LType mType;

			glm::vec3 mPosition;
			glm::vec3 mDirection;
			glm::vec3 mColor;
	};
	}
}