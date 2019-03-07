#ifndef APP_H
#define APP_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <atomic>
#include <vector>

#include "AppSettings.h"
#include "Camera.h"
#include "Geometry.h"
#include "InputController.h"
#include "RenderBackend.h"

constexpr auto WINDOW_WIDTH = 1024;
constexpr auto WINDOW_HEIGHT = 768;

#define APP_NAME "Sparkle Engine"

namespace Sparkle {
class App {
public:
    static App& getHandle()
    {
        static App handle;
        return handle;
    }

    void run(std::string config)
    {
        initialize(config);
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

    std::shared_ptr<Sparkle::RenderBackend> pRenderer;

    int windowWidth = WINDOW_WIDTH;
    int windowHeight = WINDOW_HEIGHT;

    GUI::FrameData frameData {};

    App()
    {
        pSettings = loadFromDefault();
    }
    App(const App& app) = delete;

    /* 
		Function prototypes
		*/
    void initialize(std::string config);

    std::unique_ptr<Settings> loadFromDefault() const
    {
        return std::make_unique<Settings>("assets/settings.ini");
    }
    void createWindow();
    void mainLoop();

    void cleanup();

    void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};
}

#endif
