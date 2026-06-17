#pragma once

#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include "framework/nlohmann/json.hpp"
#include "debug_ostream.h"

struct ScoreSummary
{
    std::string jsonname;
    std::string musicname;
    std::string musicauthor;
    std::string scoreauthor;
    float difficulty = 0.0f;
    float bpm = 0.0f;
    std::string thumbnail;
    std::string music;
};

inline std::vector<ScoreSummary> LoadScoreSummaries(const std::string& directoryPath = "asset\\score")
{
    std::vector<ScoreSummary> summaries;

    const std::string pattern = directoryPath + "\\*.json";
    WIN32_FIND_DATAA findData = {};
    HANDLE findHandle = FindFirstFileA(pattern.c_str(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE)
    {
        return summaries;
    }

    do
    {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            continue;
        }

        const std::string jsonName = findData.cFileName;
        const std::string jsonPath = directoryPath + "\\" + jsonName;

        std::ifstream file(jsonPath);
        if (!file.is_open())
        {
            continue;
        }

        try
        {
            nlohmann::json jsonData;
            file >> jsonData;

            ScoreSummary summary;
            summary.jsonname = jsonName;
            hal::dout << summary.jsonname << std::endl;
            summary.musicname = jsonData.value("musicname", "");
            hal::dout << summary.musicname << std::endl;
            summary.musicauthor = jsonData.value("musicauthor", "");
            summary.scoreauthor = jsonData.value("scoreauthor", "");
            summary.difficulty = jsonData.value("difficulty", 0.0f);
            summary.bpm = jsonData.value("bpm", 0.0f);
            summary.thumbnail = jsonData.value("thumbnail", "");
            summary.music = jsonData.value("music", "");

            summaries.push_back(summary);
        }
        catch (...)
        {
            continue;
        }
    } while (FindNextFileA(findHandle, &findData) != 0);

    FindClose(findHandle);

    std::sort(summaries.begin(), summaries.end(), [](const ScoreSummary& a, const ScoreSummary& b)
    {
        if (a.difficulty == b.difficulty)
        {
            return a.jsonname < b.jsonname;
        }
        return a.difficulty > b.difficulty;
    });

    return summaries;
}