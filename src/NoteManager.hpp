#pragma once
#include "Note.hpp"
#include <vector>
#include <iomanip>
#include <istream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <ctime>
namespace fs = std::filesystem;

class NoteManager
{
public:
	std::vector<Note> notes;
	std::string notesDirectory;
	NoteManager(const std::string& dir) : notesDirectory(dir)
	{
		if (!fs::exists(notesDirectory))
		{
			fs::create_directory(notesDirectory);
		}
		refreshNotes();
	}
	void refreshNotes()
	{
		notes.clear();
		for (const auto& entry : fs::directory_iterator(notesDirectory))
		{
			if (entry.path().extension() == ".txt" || entry.path().extension() == ".md")
			{
				Note newNote;
				newNote.title = entry.path().filename().string();
				newNote.filepath = entry.path().string();
				std::ifstream file(entry.path());
				std::stringstream buffer;
				buffer << file.rdbuf();
				newNote.content = buffer.str();
				auto ftime = fs::last_write_time(entry);
				newNote.rawTime = ftime;

				auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
					ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
				);
				std::time_t tt = std::chrono::system_clock::to_time_t(sctp);

				std::tm timeinfo = {};
#ifdef _WIN32
				localtime_s(&timeinfo, &tt);
#else
				localtime_r(&tt, &timeinfo);
#endif

				std::stringstream timeSS;
				timeSS << std::put_time(&timeinfo, "%Y-%m-%d %H:%M");
				newNote.displayTime = timeSS.str();
				notes.emplace_back(newNote);
			}
		}
		std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b)
			{
				return a.rawTime > b.rawTime;
			});
	}
	Note* createNote(const std::string& title)
	{
		std::string safeTitle = title;
		if (safeTitle.find(".md") == std::string::npos) safeTitle += ".md";
		Note newNote;
		newNote.title = safeTitle;
		newNote.content = "# " + title + "\n\nStart writing...";
		newNote.filepath = notesDirectory + "/" + safeTitle;
		newNote.save();
		notes.emplace_back(newNote);
		return &notes.back();
	}
	void deleteNote(int index)
	{
		if (index >= 0 && index < (int)notes.size())
		{
			fs::remove(notes[index].filepath);
			notes.erase(notes.begin() + index);
		}
	}
	bool renameNote(int index, const std::string& newTitle)
	{
		if (index < 0 || index >= (int)notes.size()) return false;
		Note& note = notes.at(index);
		std::string safeTitle = newTitle;
		if (safeTitle.find(".md") == std::string::npos) safeTitle += ".md";
		std::string newPath = notesDirectory + "/" + safeTitle;
		try
		{
			fs::rename(note.filepath, newPath);
			note.title = safeTitle;
			note.filepath = newPath;
			return true;
		}
		catch (const fs::filesystem_error& e)
		{
			std::cerr << "Rename failed: " << e.what() << std::endl;
			return false;
		}
		return false;
	}
};