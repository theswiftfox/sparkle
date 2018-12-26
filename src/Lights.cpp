#include "Lights.h"

using namespace Engine;

Lights::Light::Light(LType type, glm::vec3 loc, glm::vec3 col) : mType(type), mColor(col) {
	switch (type) {
	case SPARKLE_LIGHT_TYPE_DIRECTIONAL:
		mDirection = loc;
		mPosition = glm::vec3(0.0f);
		break;
	case SPARKLE_LIGHT_TYPE_POINT:
		mDirection = glm::vec3(0.0f);
		mPosition = loc;
		break;
	default:
		mDirection = glm::vec3(0.0f);
		mPosition = mDirection;
		mType = SPARKLE_LIGHT_TYPE_UNDEFINED;
	}
}