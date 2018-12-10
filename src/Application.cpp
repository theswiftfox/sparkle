#include "Application.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

using namespace Engine;

void App::createWindow() {
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("GLFW initialization failed!");
	}

	// do not create OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	pWindow = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME, nullptr, nullptr);
	if (!pWindow) {
		throw std::runtime_error("Window creation failed!");
	}

	// pass user pointer to this class to glfw so we can use non static member functions within callbacks
	glfwSetWindowUserPointer(pWindow, this);

	const auto mouseClicked = [](GLFWwindow* w, int b, int a, int m) {
		static_cast<App*>(glfwGetWindowUserPointer(w))->mouseButtonCallback(w, b, a, m);
	};
	glfwSetMouseButtonCallback(pWindow, mouseClicked);
}

void App::mainLoop() {
	double deltaT = 0.0;
	auto lastTime = glfwGetTime();
	auto fpsTimer = lastTime;
	auto lastFrameTime = 0.0;
	size_t frames = 0;

	double updateFreq = -1.0;
	while (!glfwWindowShouldClose(pWindow)) {
		const auto currentTime = glfwGetTime();
		deltaT = currentTime - lastTime;
		++frames;

		if (currentTime - fpsTimer >= 1.0) {
			lastFPS = frames;
			frames = 0;
			fpsTimer = currentTime;
		}
				
		glfwPollEvents();

		pCamera->update(deltaT);
		pRenderer->updateUniforms();

		pRenderer->draw(deltaT);

		lastTime = currentTime;
	}
}

void App::cleanup() {
	pScene->cleanup();

	pRenderer->cleanup();

	glfwDestroyWindow(pWindow);
	glfwTerminate();
}


Geometry::Mesh::BufferOffset App::storeMesh(const Geometry::Mesh* mesh) {
	return pRenderer->storeMesh(mesh);
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// ignore mouse events if:
	// todo
}