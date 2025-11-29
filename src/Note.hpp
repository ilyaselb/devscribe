#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>

struct Note
{
	std::string title;
	std::string content;
	std::string filepath;
	bool isDirty = false;
	std::filesystem::file_time_type rawTime;
	std::string displayTime;

	bool save()
	{
		if (filepath.empty()) return false;

		std::ofstream out(filepath);
		if (out.is_open())
		{
			out << content;
			out.close();
			isDirty = false;
			return true;
		}
		return false;
	}
};