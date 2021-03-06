#include "DebugInfoRenderer.hpp"
#include <glad/glad.h>
#include <imgui/sdl/imgui_impl_sdl.h>
#include <imgui/opengl3/imgui_impl_opengl3.h>
#include <iostream>
#include "Vertex.hpp"

using namespace std;

DebugInfoRenderer::DebugInfoRenderer(std::unique_ptr<Window> &debugWindow) : debugWindow(debugWindow), backgroundColor(ImVec4(255/255.0f, 182/255.0f, 193/255.0f, 1.00f)) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO(); (void)io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;
    ImGui_ImplSDL2_InitForOpenGL(debugWindow->getWindowRef(), debugWindow->getGLContext());
    ImGui_ImplOpenGL3_Init();
}

DebugInfoRenderer::~DebugInfoRenderer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

void DebugInfoRenderer::update(vector<string> biosFunctionsLog) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(debugWindow->getWindowRef());
    Dimensions windowDimensions = debugWindow->getDimensions();
    ImVec2 syscallWindowSize = ImVec2(static_cast<float>(((windowDimensions.width / 3) * 2) - 20), static_cast<float>(windowDimensions.height - 20));
    ImGui::NewFrame();
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(syscallWindowSize, ImGuiCond_Always);
        ImGui::Begin("BIOS function calls", NULL, ImGuiWindowFlags_NoResize);
        {
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
            ImGui::BeginChild("Child1", ImVec2(ImGui::GetWindowContentRegionWidth(), static_cast<float>(windowDimensions.height - 60)), false, window_flags);
            for (unsigned int i = 0; i < biosFunctionsLog.size(); i++) {
                ImGui::Text("%s", biosFunctionsLog[i].c_str());
            }
            ImGui::SetScrollHereY();
            ImGui::EndChild();
        }
        ImGui::End();
    }
    ImGui::Render();
    glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
    glClearColor(backgroundColor.x, backgroundColor.y, backgroundColor.z, backgroundColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DebugInfoRenderer::handleSDLEvent(SDL_Event event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}
