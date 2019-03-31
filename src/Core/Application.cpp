#include "Application.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace Sparkle;

void App::initialize(std::string config)
{
    if (!config.empty()) {
        pSettings = std::make_unique<Settings>(config);
    } else {
        pSettings = loadFromDefault();
    }
    if (!pSettings->load()) {
        std::cout << "Error loading settings" << std::endl;
    }
    auto resolution = pSettings->getResolution();
    windowWidth = resolution.first;
    windowHeight = resolution.second;

    pCamera = std::make_unique<Camera>(pSettings);
    createWindow();

    pRenderer = std::make_unique<Sparkle::RenderBackend>(pWindow, APP_NAME, pCamera);
    pRenderer->initialize(pSettings, pSettings->withValidationLayer());

	pSceneLoader = std::make_unique<Import::SceneLoader>();
	pSceneLoader->loadFromFile(pSettings->getLevelPath());

    pInputController = std::make_shared<InputController>(pWindow, pCamera);
}

void App::createWindow()
{
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

void App::mainLoop()
{
    double deltaT = 0.0;
    auto lastTime = glfwGetTime();
    auto fpsTimer = lastTime;
    auto lastFrameTime = 0.0;
    size_t frames = 0;
    double mx, my;
	ImGuiIO& io = ImGui::GetIO();

	glm::vec3 initialCameraPos = glm::vec3(16.0f, 90.0f, -50.0f);
	pCamera->setPosition(initialCameraPos);
	//pCamera->setAngle(28.0f, 165.f);

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

        if (pSceneLoader->isLoaded() && !pScene) {
            pScene = pSceneLoader->processScene();
            pRenderer->updateScenePtr(pScene);
        }

        // Update imGui
        ImGui::NewFrame();
        io.DisplaySize = ImVec2((float)windowWidth, (float)windowHeight);
        io.DeltaTime = (float)deltaT;
        glfwGetCursorPos(pWindow, &mx, &my);
        io.MousePos = ImVec2((float)mx, (float)my);

        pInputController->update((float)deltaT);
		bool updatedCam = false;
        if (pCamera->changed()) {
            pCamera->update((float)deltaT);
			updatedCam = true;
        }
		pRenderer->updateUniforms(updatedCam);
        pRenderer->updateUiData(frameData);
        pRenderer->draw(deltaT);

        lastTime = currentTime;
    }
}

void App::cleanup()
{
    pRenderer->cleanup();

    ImGui::DestroyContext();
    glfwDestroyWindow(pWindow);
    glfwTerminate();
}

Geometry::Mesh::BufferOffset App::uploadMeshGPU(const Geometry::Mesh* mesh)
{
    return pRenderer->uploadMeshGPU(mesh);
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    // ignore mouse events if:
    // todo
}