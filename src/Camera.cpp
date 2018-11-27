#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

using namespace Engine;

Camera::Camera(std::shared_ptr<Settings> settings) {
	auto [width, height] = settings->getResolution();
	nearZ = NEAR_Z;
	farZ = settings->getRenderDistance();
	projMat = glm::perspective(glm::radians(settings->getFov()), (float)width / (float)height, nearZ, farZ);
	gameSettings = settings;
}

float Camera::nearPlane() const {
	return nearZ;
}
float Camera::farPlane() const {
	return farZ;
}

void Camera::updateAspect(float aspect) {
	projMat = glm::perspective(glm::radians(gameSettings->getFov()), aspect, NEAR_Z, gameSettings->getRenderDistance());
}

void Camera::resetAspect() {
	auto[width, height] = gameSettings->getResolution();
	projMat = glm::perspective(glm::radians(gameSettings->getFov()), (float)width / (float)height, NEAR_Z, gameSettings->getRenderDistance());
}

glm::mat4 Camera::getView() const {
	return viewMat;
}

glm::mat4 Camera::getProjection() const {
	return projMat;
}

glm::mat4 Camera::getViewProjectionMatrix() const {
	return projMat * viewMat;
}

glm::vec3 Camera::getPosition() const {
	return cameraPos;
}

glm::vec3 Camera::getRightWorld() {
	return glm::vec3(viewMat[0][0], viewMat[1][0], viewMat[2][0]);
}

glm::vec3 Camera::getUpWorld() {
	return glm::vec3(viewMat[0][1], viewMat[1][1], viewMat[2][1]);
}

void Camera::update(float deltaT) {
	const auto direction = glm::vec3(
		cos(-vAngle) * sin(-hAngle),
		sin(-vAngle),
		cos(-vAngle) * cos(-hAngle)
	);

	hRotation = glm::rotate(glm::mat4(1.f), hAngle, glm::vec3(0, 1, 0));

	viewMat = glm::lookAt(
		cameraPos,
		cameraPos + direction, //
		glm::vec3(0, 1, 0)
	);
	lasthAngle = hAngle;
}


void Camera::moveForward(float deltaT) {
	glm::vec4 moveVec(0.f, 0.f, playerSpeed * deltaT, 1);
	moveVec = moveVec * hRotation;
	cameraPos = cameraPos + glm::vec3(moveVec);
}


void Camera::moveBackward(float deltaT) {
	glm::vec4 moveVec(0.f, 0.f, -PLAYER_DEFAULT_SPEED * PLAYER_REVERSING_FACTOR * deltaT, 1);
	moveVec = moveVec * hRotation;
	cameraPos = cameraPos + glm::vec3(moveVec);
}

void Camera::moveRight(float deltaT) {
	glm::vec4 moveVec(-playerSpeed * PLAYER_SIDESTEP_FACTOR * deltaT, 0.f, 0.f, 1);
	moveVec = moveVec * hRotation;
	cameraPos = cameraPos + glm::vec3(moveVec);
}

void Camera::moveLeft(float deltaT) {
	glm::vec4 moveVec(playerSpeed * PLAYER_SIDESTEP_FACTOR * deltaT, 0.f, 0.f, 1);
	moveVec = moveVec * hRotation;
	cameraPos = cameraPos + glm::vec3(moveVec);
}

void Camera::setAngle(float x, float y) {
	hAngle += x * MOUSE_SPEED;
	vAngle += y * MOUSE_SPEED;
	if (vAngle <= -1.5) vAngle = -1.5;
	if (vAngle >= 1.5) vAngle = 1.5;
}