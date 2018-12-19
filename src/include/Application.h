#ifndef APP_H
#define APP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <atomic>
#include <vector>

#include "InputController.h"
#include "RenderBackend.h"
#include "AppSettings.h"
#include "Geometry.h"
#include "Camera.h"

constexpr auto WINDOW_WIDTH = 1024;
constexpr auto WINDOW_HEIGHT = 768;

#define APP_NAME "Sparkle Engine"

namespace Engine {
	class App {
	public:
		static App& getHandle() {
			static App handle;
			return handle;
		}

		void run(std::string config, bool withValidation = false) {
			initialize(config, withValidation);
			mainLoop();
			cleanup();
		}

		Geometry::Mesh::BufferOffset uploadMeshGPU(const Geometry::Mesh* m);

		std::shared_ptr<RenderBackend> getRenderBackend() { return pRenderer; }

	private:
		/*
		Members
		*/
		GLFWwindow* pWindow;

		std::shared_ptr<Settings> pSettings;
		std::shared_ptr<Camera> pCamera;
		std::shared_ptr<InputController> pInputController = nullptr;
		std::shared_ptr<Geometry::Scene> pScene;

		std::shared_ptr<Engine::RenderBackend> pRenderer;

		int windowWidth = WINDOW_WIDTH;
		int windowHeight = WINDOW_HEIGHT;

		size_t lastFPS;

		App() {
			pSettings = loadFromDefault();
		}
		App(const App& app) = delete;

		/* 
		Function prototypes
		*/
		void initialize(std::string config, bool withValidation) {
			if (!config.empty()) {
				pSettings = std::make_unique<Settings>(config);
			} else {
				pSettings = loadFromDefault();
			}
			auto resolution = pSettings->getResolution();
			windowWidth = resolution.first;
			windowHeight = resolution.second;

			pCamera = std::make_unique<Camera>(pSettings);
			pScene = std::make_shared<Geometry::Scene>();

			createWindow();

			pRenderer = std::make_unique<Engine::RenderBackend>(pWindow, APP_NAME, pCamera, pScene);
			pRenderer->initialize(pSettings, withValidation);

			pInputController = std::make_shared<InputController>(pWindow, pCamera);
		}

		std::unique_ptr<Settings> loadFromDefault() const {
			return std::make_unique<Settings>("assets/settings.ini");
		}
		void createWindow();
		void mainLoop();

		void cleanup();

		void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	};
}

#endif
