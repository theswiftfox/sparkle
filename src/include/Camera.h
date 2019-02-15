#ifndef CAMERA_H
#define CAMERA_H

#define NEAR_Z 0.1f

#define PLAYER_DEFAULT_SPEED 30.0f // base movement speed

#define MOUSE_SPEED 0.00390625f

#include "AppSettings.h"
#include <glm/glm.hpp>

#include <memory>

#define _USE_MATH_DEFINES
#include <math.h>

class Camera {
public:
    Camera(std::shared_ptr<Sparkle::Settings> settings);
    void updateAspect(float aspectRatio);
    void resetAspect();

    void update(float deltaT);
    void moveForward(float deltaT);
    void moveBackward(float deltaT);
    void moveLeft(float deltaT);
    void moveRight(float deltaT);
    void setAngle(float x, float y);

    glm::vec3 getPosition() const;
    glm::mat4 getView() const;
    glm::mat4 getProjection() const;
    glm::mat4 getViewProjectionMatrix() const;
    glm::vec3 getFrontWorld();
    glm::vec3 getRightWorld();
    glm::vec3 getUpWorld();

    float nearPlane() const;
    float farPlane() const;

    /*
	* @Brief: Indicates if the Camera has changed since the last call to update
	*/
    bool changed() const;

private:
    std::shared_ptr<Sparkle::Settings> gameSettings;

    float hAngle = (float)M_PI_4;
    float vAngle = 0.0f;
    float lasthAngle = hAngle;

    float nearZ;
    float farZ;

    bool hasChanged = true;

    glm::vec3 cameraPos;
    glm::mat4 viewMat;
    glm::mat4 projMat;

    float playerSpeed = PLAYER_DEFAULT_SPEED;
};

#endif