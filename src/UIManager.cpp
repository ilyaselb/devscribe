#include "UIManager.hpp"
#include "imgui_markdown.h"
#include <cstring>
#include <algorithm>
#include <cctype>
#include <iostream>

UIManager::UIManager(NoteManager& nm) : 
    noteManager(nm), selectedNoteIndex(-1), openDeletePopup(false), 
    openRenamePopup(false), noteIndexToRename(-1), notificationDuration(0.0f), 
    isPreviewMode(false)
{
    editorBuffer = new char[EDITOR_BUFFER_SIZE];
    std::memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
    std::memset(searchBuffer, 0, sizeof(searchBuffer));
}

UIManager::~UIManager()
{
    delete[] editorBuffer;
}

void UIManager::CopyToBuffer(const std::string& source, char* buffer, size_t bufferSize)
{
    size_t copyLen = std::min(source.length(), bufferSize - 1);
    std::memcpy(buffer, source.c_str(), copyLen);
    buffer[copyLen] = '\0';
}

bool UIManager::StringContainsCaseInsensitive(
    const std::string& haystack, 
    const std::string& needle)
{
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return (it != haystack.end());
}

void UIManager::Render()
{
    RenderDockSpace();
    RenderNoteList();
    RenderEditorOrPreview();
    RenderPopups();
    RenderNotifications();
}

void UIManager::RenderDockSpace()
{
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
}

void UIManager::RenderNoteList()
{
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
            std::memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
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
#ifdef _WIN32
            strcpy_s(newTitle, sizeof(newTitle), "Untitled");
#else
            strncpy(newTitle, "Untitled", sizeof(newTitle));
#endif
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::InputText("Title", newTitle, sizeof(newTitle), ImGuiInputTextFlags_EnterReturnsTrue);

        if (ImGui::Button("Create") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter)))
        {
            noteManager.createNote(newTitle);
            ImGui::CloseCurrentPopup();
            noteManager.refreshNotes();
            selectedNoteIndex = (int)noteManager.notes.size() - 1;
            std::memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
            CopyToBuffer(noteManager.notes.back().content, editorBuffer, EDITOR_BUFFER_SIZE);
            std::memset(newTitle, 0, sizeof(newTitle));
            ShowNotification("Note Created");
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void UIManager::RenderEditorOrPreview()
{
    ImGui::Begin("Editor");

    bool hasValidSelection = (selectedNoteIndex >= 0 && selectedNoteIndex < (int)noteManager.notes.size());

    if (hasValidSelection)
    {
        Note& currentNote = noteManager.notes[selectedNoteIndex];

        if (ImGui::Button("Save"))
        {
            currentNote.content = std::string(editorBuffer);
            if (currentNote.save())
            {
                ShowNotification("Note Saved");
            }
        }
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
        if (ImGui::Button("Delete"))
        {
            openDeletePopup = true;
        }
        ImGui::PopStyleColor(1);

        ImGui::SameLine();
        if (ImGui::Checkbox("Preview Mode", &isPreviewMode))
        {
            if (isPreviewMode)
            {
                currentNote.content = std::string(editorBuffer);
            }
        }

        ImGui::SameLine();
        ImGui::TextDisabled("| %s", currentNote.filepath.c_str());
        ImGui::Separator();

        if (isPreviewMode)
        {
            RenderMarkdown();
        }
        else
        {
            ImGui::InputTextMultiline("##source", editorBuffer, EDITOR_BUFFER_SIZE,
                ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);

            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
            {
                currentNote.content = std::string(editorBuffer);
                if (currentNote.save())
                {
                    ShowNotification("Note Saved");
                }
            }
        }
    }
    else
    {
        ImGui::TextDisabled("Select a note to edit...");
    }

    ImGui::End();
}

void UIManager::RenderMarkdown()
{
    if (selectedNoteIndex < 0 || selectedNoteIndex >= (int)noteManager.notes.size()) return;
    
    const std::string& markdownText = noteManager.notes[selectedNoteIndex].content;

    ImGui::MarkdownConfig mdConfig;
    ImGui::Markdown(markdownText.c_str(), markdownText.length(), mdConfig);
}

void UIManager::RenderPopups()
{
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
                std::memset(editorBuffer, 0, EDITOR_BUFFER_SIZE);
                ShowNotification("Note Deleted");
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (openRenamePopup) { ImGui::OpenPopup("Rename Note"); openRenamePopup = false; }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Rename Note", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static char renameBuffer[256] = "";

        if (ImGui::IsWindowAppearing())
        {
            std::memset(renameBuffer, 0, sizeof(renameBuffer));
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
                if (noteManager.renameNote(noteIndexToRename, std::string(renameBuffer)))
                {
                    ShowNotification("Note Renamed");
                }
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            std::memset(renameBuffer, 0, sizeof(renameBuffer));
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void UIManager::ShowNotification(const std::string& message, float duration)
{
    notificationMessage = message;
    notificationDuration = duration;
}

void UIManager::RenderNotifications()
{
    if (notificationDuration > 0.0f)
    {
        notificationDuration -= ImGui::GetIO().DeltaTime;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 window_pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 20.0f, viewport->WorkPos.y + viewport->WorkSize.y - 20.0f);
        ImVec2 window_pos_pivot = ImVec2(1.0f, 1.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.7f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | 
            ImGuiWindowFlags_NoMove;
        
        if (ImGui::Begin("Notification", NULL, flags))
        {
            ImGui::Text("%s", notificationMessage.c_str());
            ImGui::End();
        }
    }
}
