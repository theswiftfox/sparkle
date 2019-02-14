#include "Camera.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

using namespace Engine;

Camera::Camera(std::shared_ptr<Settings> settings)
    : cameraPos(0.0f)
{
    auto [width, height] = settings->getResolution();
    nearZ = NEAR_Z;
    farZ = settings->getRenderDistance();
    projMat = glm::perspective(glm::radians(settings->getFov()), (float)width / (float)height, nearZ, farZ);
    projMat[1, 1] *= -1.0f;
    gameSettings = settings;
}

float Camera::nearPlane() const
{
    return nearZ;
}
float Camera::farPlane() const
{
    return farZ;
}

bool Camera::changed() const
{
    return hasChanged;
}

void Camera::updateAspect(float aspect)
{
    projMat = glm::perspective(glm::radians(gameSettings->getFov()), aspect, NEAR_Z, gameSettings->getRenderDistance());
}

void Camera::resetAspect()
{
    auto [width, height] = gameSettings->getResolution();
    projMat = glm::perspective(glm::radians(gameSettings->getFov()), (float)width / (float)height, NEAR_Z, gameSettings->getRenderDistance());
}

glm::mat4 Camera::getView() const
{
    return viewMat;
}

glm::mat4 Camera::getProjection() const
{
    return projMat;
}

glm::mat4 Camera::getViewProjectionMatrix() const
{
    return projMat * viewMat;
}

glm::vec3 Camera::getPosition() const
{
    return cameraPos;
}

glm::vec3 Camera::getRightWorld()
{
    return glm::vec3(viewMat[0][0], viewMat[1][0], viewMat[2][0]);
}

glm::vec3 Camera::getFrontWorld()
{
    return glm::vec3(viewMat[0][2], viewMat[1][2], viewMat[2][2]);
}

glm::vec3 Camera::getUpWorld()
{
    return glm::vec3(viewMat[0][1], viewMat[1][1], viewMat[2][1]);
}

void Camera::update(float deltaT)
{
    const auto direction = glm::vec3(
        cos(-vAngle) * sin(-hAngle),
        sin(-vAngle),
        cos(-vAngle) * cos(-hAngle));

    viewMat = glm::lookAt(
        cameraPos,
        cameraPos + direction, //
        glm::vec3(0, 1, 0));
    lasthAngle = hAngle;

    hasChanged = false;
}

void Camera::moveForward(float deltaT)
{
    cameraPos = cameraPos + -playerSpeed * deltaT * getFrontWorld();
    hasChanged = true;
}

void Camera::moveBackward(float deltaT)
{
    cameraPos = cameraPos + playerSpeed * deltaT * getFrontWorld();
    hasChanged = true;
}

void Camera::moveRight(float deltaT)
{
    cameraPos = cameraPos + playerSpeed * deltaT * getRightWorld();
    hasChanged = true;
}

void Camera::moveLeft(float deltaT)
{
    cameraPos = cameraPos + -playerSpeed * deltaT * getRightWorld();
    hasChanged = true;
}

void Camera::setAngle(float x, float y)
{
    hAngle += x * MOUSE_SPEED;
    vAngle -= y * MOUSE_SPEED;
    if (vAngle <= -1.5)
        vAngle = -1.5;
    if (vAngle >= 1.5)
        vAngle = 1.5;
    hasChanged = true;
}