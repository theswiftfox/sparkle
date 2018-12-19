#ifndef CAMERA_H
#define CAMERA_H

#define NEAR_Z 0.1f

#define PLAYER_DEFAULT_SPEED 30.0f		// base movement speed
#define PLAYER_MAX_SPEED 90.0f			// maximum run speed for character
#define PLAYER_SPEED_DAMPENING 20.0f	// dampening factor for movement penalty
#define PLAYER_SIDESTEP_FACTOR 0.6f		// factor for sideways character movement
#define PLAYER_REVERSING_FACTOR 0.4f	// factor for backwards character movement
#define PLAYER_VELOCITY_TRESHOLD 10.0f	// minimum total absolute velocity on x and y for movementspeed increase
#define PLAYER_HEIGHT 10.0f

#define PLAYER_JUMP_POWER 60.0f
#define MIN_FALL_SPEED 30.0f
#define MAX_FALL_SPEED 60.0f

#define MOUSE_SPEED 0.00390625f

#include "AppSettings.h"
#include <glm/glm.hpp>

#include <memory>

#define _USE_MATH_DEFINES  
#include <math.h>  

class Camera {
public:
	Camera(std::shared_ptr<Engine::Settings> settings);
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

private:
	std::shared_ptr<Engine::Settings> gameSettings;

	float hAngle = (float)M_PI_4;
	float vAngle = 0.0f;
	float lasthAngle = hAngle;

	float nearZ;
	float farZ;

	glm::vec3 cameraPos;
	glm::mat4 hRotation;
	glm::mat4 viewMat;
	glm::mat4 projMat;
	
	bool isCrouching = false;
	bool falling = false;

	float playerSpeed = PLAYER_DEFAULT_SPEED;
	float fallSpeed = MIN_FALL_SPEED;
};

#endif