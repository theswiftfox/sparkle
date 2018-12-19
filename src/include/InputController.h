#pragma once

#include <glfw/glfw3.h>
#include "Camera.h"

class InputController {
public:
	InputController(GLFWwindow* window, std::shared_ptr<Camera> camera) {
		this->window = window;
		this->camera = camera;

		init();
	}

	void update(float deltaT);
	void init();

	void glfwCallback(GLFWwindow* window, int button, int action, int mods);

private:
	std::shared_ptr<Camera> camera;
	GLFWwindow* window;

	bool isTurning = false;

};
