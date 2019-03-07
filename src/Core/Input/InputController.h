#pragma once

#include <vector>

#include "Camera.h"
#include <GLFW/glfw3.h>

class InputController {
public:
    InputController(GLFWwindow* window, std::shared_ptr<Camera> camera)
    {
        this->window = window;
        this->camera = camera;

        init();
    }

    void update(float deltaT);
    void init();

    void glfwCallback(GLFWwindow* window, int button, int action, int mods);
    void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    bool handlingMouse() const { return isTurning; }

private:
    std::shared_ptr<Camera> camera;
    GLFWwindow* window;
    std::vector<bool> mouseWasPressed;

    bool isTurning = false;
};
