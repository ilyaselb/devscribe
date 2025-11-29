#pragma once
#include "Note.hpp"
#include <vector>
#include <string>
#include <filesystem>

class NoteManager
{
public:
    std::vector<Note> notes;
    std::string notesDirectory;

    NoteManager(const std::string& dir);
    void refreshNotes();
    Note* createNote(const std::string& title);
    void deleteNote(int index);
    bool renameNote(int index, const std::string& newTitle);
};