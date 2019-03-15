#include "InputController.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __linux__
#ifdef USE_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#else
#define GLFW_EXPOSE_NATIVE_X11
#endif
#endif

#include <GLFW/glfw3native.h>
#include <imgui/imgui.h>

#include "Application.h"

void InputController::glfwCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            isTurning = true;
            glfwSetCursorPos(window, 0, 0);
        } else if (action == GLFW_RELEASE) {
            isTurning = false;
        }
    }
    if (action == GLFW_PRESS && button >= 0 && button < mouseWasPressed.size()) {
        mouseWasPressed[button] = true;
    }
}

void InputController::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_F1) {
        if (action == GLFW_RELEASE) {
            Sparkle::App::getHandle().getRenderBackend()->getUiHandle()->toggleOptions();
        }
    }
	if (key == GLFW_KEY_F3) {
		if (action == GLFW_RELEASE) {
			Sparkle::App::getHandle().getRenderBackend()->toggleCPUCullEnabled();
		}
	}
	if (key == GLFW_KEY_F4) {
		if (action == GLFW_RELEASE) {
			Sparkle::App::getHandle().getRenderBackend()->toggleComputeEnabled();
		}
	}
	if (key == GLFW_KEY_F5) {
		if (action == GLFW_RELEASE) {
			Sparkle::App::getHandle().getRenderBackend()->reloadShaders();
		}
	}
}

void InputController::update(float deltaT)
{
    // mouse
    if (isTurning) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        camera->setAngle(static_cast<float>(x), -static_cast<float>(y));
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

    // imgui
    auto& io = ImGui::GetIO();
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
    if (focused) {
        for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) {
            // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
            io.MouseDown[i] = mouseWasPressed[i] || glfwGetMouseButton(window, i) != 0;
            mouseWasPressed[i] = false;
        }

        if (io.WantSetMousePos) {
            glfwSetCursorPos(window, (double)mouse_pos_backup.x, (double)mouse_pos_backup.y);
        } else {
            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
        }
    }
}

void InputController::init()
{
    auto& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos; // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "inputController";

#ifdef _WIN32
    io.ImeWindowHandle = (void*)glfwGetWin32Window(window);
#elif __linux__
    io.ImeWindowHandle = (void*)glfwGetX11Window(window);
#endif

    mouseWasPressed.resize(IM_ARRAYSIZE(io.MouseDown));
    glfwSetWindowUserPointer(window, this);
    auto mouseClicked = [](GLFWwindow* w, int b, int a, int m) {
        static_cast<InputController*>(glfwGetWindowUserPointer(w))->glfwCallback(w, b, a, m);
    };
    auto keyPressed = [](GLFWwindow* w, int k, int s, int a, int m) {
        static_cast<InputController*>(glfwGetWindowUserPointer(w))->glfwKeyCallback(w, k, s, a, m);
    };
    glfwSetMouseButtonCallback(window, mouseClicked);
    glfwSetKeyCallback(window, keyPressed);
}