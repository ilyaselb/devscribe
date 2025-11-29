#pragma once
#include <GLFW/glfw3.h>
#include <memory>
#include "NoteManager.hpp"
#include "UIManager.hpp"

class App
{
public:
    App();
    ~App();

    bool Init();
    void Run();

private:
    GLFWwindow* window;
    std::unique_ptr<NoteManager> noteManager;
    std::unique_ptr<UIManager> uiManager;

    void SetupStyle();
};
