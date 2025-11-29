#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <algorithm>
#include <cctype>

#include "NoteManager.hpp" 

#define EDITOR_BUFFER_SIZE (1024 * 256)

void CopyToBuffer(const std::string& source, char* buffer, size_t bufferSize)
{
    size_t copyLen = std::min(source.length(), bufferSize - 1);
    std::memcpy(buffer, source.c_str(), copyLen);
    buffer[copyLen] = '\0';
}

bool StringContainsCaseInsensitive(const std::string& haystack, const std::string& needle)
{
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return (it != haystack.end());
}

int main()
{
    if (!glfwInit()) return -1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "DevScribe", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    NoteManager noteManager("notes");

    char* editorBuffer = new char[EDITOR_BUFFER_SIZE];
    memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);

    char searchBuffer[128] = "";
    int selectedNoteIndex = -1;

    bool openDeletePopup = false;
    bool openRenamePopup = false;
    int noteIndexToRename = -1;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (ImGui::Begin("DockSpace Window", nullptr, window_flags))
        {
            ImGui::PopStyleVar(3);
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        }
        else
        {
            ImGui::PopStyleVar(3);
        }
        ImGui::End();

        ImGui::Begin("Note List");

        if (ImGui::Button("New Note", ImVec2(-1, 0)))
        {
            ImGui::OpenPopup("NewNotePopup");
        }

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##search", "Search notes...", searchBuffer, sizeof(searchBuffer));

        ImGui::Separator();

        for (int i = 0; i < (int)noteManager.notes.size(); i++)
        {
            if (searchBuffer[0] != '\0' &&
                !StringContainsCaseInsensitive(noteManager.notes[i].title, searchBuffer))
            {
                continue;
            }

            bool isSelected = (selectedNoteIndex == i);

            if (ImGui::Selectable(noteManager.notes[i].title.c_str(), isSelected))
            {
                selectedNoteIndex = i;
                memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
                CopyToBuffer(noteManager.notes[i].content, editorBuffer, EDITOR_BUFFER_SIZE);
            }

            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Rename"))
                {
                    noteIndexToRename = i;
                    openRenamePopup = true;
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            ImGui::TextDisabled("%s", noteManager.notes[i].displayTime.c_str());
        }

        if (ImGui::BeginPopup("NewNotePopup"))
        {
            static char newTitle[128] = "Untitled";
            if (ImGui::IsWindowAppearing())
            {
                strcpy_s(newTitle, sizeof(newTitle), "Untitled");
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::InputText("Title", newTitle, sizeof(newTitle), ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::Button("Create") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
            {
                noteManager.createNote(newTitle);
                ImGui::CloseCurrentPopup();
                noteManager.refreshNotes();
                selectedNoteIndex = (int)noteManager.notes.size() - 1;
                memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
                CopyToBuffer(noteManager.notes.back().content, editorBuffer, EDITOR_BUFFER_SIZE);
                memset(newTitle, 0, sizeof(newTitle));
            }
            ImGui::EndPopup();
        }

        ImGui::End();

        ImGui::Begin("Editor");

        bool hasValidSelection = (selectedNoteIndex >= 0 && selectedNoteIndex < (int)noteManager.notes.size());

        if (hasValidSelection)
        {
            Note& currentNote = noteManager.notes[selectedNoteIndex];

            if (ImGui::Button("Save"))
            {
                currentNote.content = std::string(editorBuffer);
                currentNote.save();
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
            if (ImGui::Button("Delete"))
            {
                openDeletePopup = true;
            }
            ImGui::PopStyleColor(1);

            ImGui::SameLine();
            ImGui::TextDisabled("| %s", currentNote.filepath.c_str());
            ImGui::Separator();

            ImGui::InputTextMultiline("##source", editorBuffer, EDITOR_BUFFER_SIZE,
                ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);

            if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
            {
                currentNote.content = std::string(editorBuffer);
                currentNote.save();
            }

            if (openDeletePopup) { ImGui::OpenPopup("Delete Note?"); openDeletePopup = false; }

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            if (ImGui::BeginPopupModal("Delete Note?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure you want to delete this note?");
                ImGui::Separator();
                if (ImGui::Button("Yes, Delete", ImVec2(120, 0)))
                {
                    if (selectedNoteIndex >= 0 && selectedNoteIndex < (int)noteManager.notes.size())
                    {
                        noteManager.deleteNote(selectedNoteIndex);
                        selectedNoteIndex = -1;
                        memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::TextDisabled("Select a note to edit...");
        }

        if (openRenamePopup) { ImGui::OpenPopup("Rename Note"); openRenamePopup = false; }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Rename Note", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char renameBuffer[256] = "";

            if (ImGui::IsWindowAppearing())
            {
                memset(renameBuffer, 0, sizeof(renameBuffer));
                if (noteIndexToRename >= 0 && noteIndexToRename < (int)noteManager.notes.size())
                {
                    CopyToBuffer(noteManager.notes[noteIndexToRename].title, renameBuffer, sizeof(renameBuffer));
                    ImGui::SetKeyboardFocusHere();
                }
            }

            ImGui::InputText("New Name", renameBuffer, sizeof(renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

            if (ImGui::Button("Rename") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
            {
                if (noteIndexToRename >= 0 && noteIndexToRename < (int)noteManager.notes.size())
                {
                    noteManager.renameNote(noteIndexToRename, std::string(renameBuffer));
                    selectedNoteIndex = -1;
                    memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
                    memset(renameBuffer, 0, sizeof(renameBuffer));
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                memset(renameBuffer, 0, sizeof(renameBuffer));
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
        glfwSwapBuffers(window);
    }

    delete[] editorBuffer;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}