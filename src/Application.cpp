#include "Application.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

using namespace Engine;

void App::initialize(std::string config) {
	if (!config.empty()) {
		pSettings = std::make_unique<Settings>(config);
	}
	else {
		pSettings = loadFromDefault();
	}
	if (!pSettings->load()) {
		std::cout << "Error loading settings" << std::endl;
	}
	auto resolution = pSettings->getResolution();
	windowWidth = resolution.first;
	windowHeight = resolution.second;

	pCamera = std::make_unique<Camera>(pSettings);
	pScene = std::make_shared<Geometry::Scene>();

	createWindow();

	pRenderer = std::make_unique<Engine::RenderBackend>(pWindow, APP_NAME, pCamera, pScene);
	pRenderer->initialize(pSettings, pSettings->withValidationLayer());

	pInputController = std::make_shared<InputController>(pWindow, pCamera);
}

void App::createWindow() {
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("GLFW initialization failed!");
	}

	// do not create OpenGL context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	pWindow = glfwCreateWindow(windowWidth, windowHeight, APP_NAME, nullptr, nullptr);
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
	double mx, my;

	double updateFreq = -1.0;
	while (!glfwWindowShouldClose(pWindow)) {
		const auto currentTime = glfwGetTime();
		deltaT = currentTime - lastTime;
		++frames;

		if (currentTime - fpsTimer >= 1.0) {
			frameData.fps = frames;
			frames = 0;
			fpsTimer = currentTime;
		}
				
		glfwPollEvents();

		// Update imGui
		ImGui::NewFrame();
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)windowWidth, (float)windowHeight);
		io.DeltaTime = (float)deltaT;
		glfwGetCursorPos(pWindow, &mx, &my);
		io.MousePos = ImVec2((float)mx, (float)my);

		pInputController->update((float)deltaT);
		pCamera->update((float)deltaT);
		pRenderer->updateUniforms();
		pRenderer->updateUiData(frameData);
		pRenderer->draw(deltaT);

		lastTime = currentTime;
	}
}

void App::cleanup() {
	pScene->cleanup();

	pRenderer->cleanup();

	ImGui::DestroyContext();
	glfwDestroyWindow(pWindow);
	glfwTerminate();
}


Geometry::Mesh::BufferOffset App::uploadMeshGPU(const Geometry::Mesh* mesh) {
	return pRenderer->uploadMeshGPU(mesh);
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	// ignore mouse events if:
	// todo
}