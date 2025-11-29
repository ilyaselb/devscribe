#pragma once
#include "imgui.h"
#include "NoteManager.hpp"
#include <string>

#define EDITOR_BUFFER_SIZE (1024 * 256)

class UIManager
{
public:
    UIManager(NoteManager& noteManager);
    ~UIManager();

    void Render();

private:
    NoteManager& noteManager;
    char* editorBuffer;
    char searchBuffer[128];
    int selectedNoteIndex;

    bool openDeletePopup;
    bool openRenamePopup;
    int noteIndexToRename;

    void CopyToBuffer(const std::string& source, char* buffer, size_t bufferSize);
    bool StringContainsCaseInsensitive(const std::string& haystack, const std::string& needle);

    void RenderDockSpace();
    void RenderNoteList();
    void RenderEditor();
    void RenderPopups();
    void RenderNotifications();

    // Notification state
    std::string notificationMessage;
    float notificationDuration;
    void ShowNotification(const std::string& message, float duration = 2.0f);
};
