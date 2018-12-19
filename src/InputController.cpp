#include "InputController.h"

void InputController::glfwCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			isTurning = true;
			glfwSetCursorPos(window, 0, 0);
		}
		else if (action == GLFW_RELEASE) {
			isTurning = false;
		}
	}
}

void InputController::update(float deltaT) {
	// mouse
	if (isTurning) {
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		camera->setAngle(static_cast<float>(x), static_cast<float>(y));
		glfwSetCursorPos(window, 0, 0);
	}

	// keyboard
	if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (glfwGetKey(window, GLFW_KEY_W)) {
		camera->moveForward(deltaT);
	}
	if (glfwGetKey(window, GLFW_KEY_S)) {
		camera->moveBackward(deltaT);
	}
	if (glfwGetKey(window, GLFW_KEY_A)) {
		camera->moveLeft(deltaT);
	}
	if (glfwGetKey(window, GLFW_KEY_D)) {
		camera->moveRight(deltaT);
	}
}

void InputController::init() {
	glfwSetWindowUserPointer(window, this);
	auto mouseClicked = [](GLFWwindow* w, int b, int a, int m) {
		static_cast<InputController*>(glfwGetWindowUserPointer(w))->glfwCallback(w, b, a, m);
	};
	glfwSetMouseButtonCallback(window, mouseClicked);
}