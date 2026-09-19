#pragma once

#include "nlohmann/json.hpp"

#include <filesystem>
#include <string>

namespace JSON {

    struct FileWriteOptions
    {
        int indent{ 4 };
        bool trailingNewline{ false };
    };

    bool ReadFile(const std::filesystem::path& a_path, nlohmann::json& a_data, const char* a_label, bool a_allowComments = true);

    bool WriteFile(const std::filesystem::path& a_path, const nlohmann::json& a_data, const char* a_label, std::string* a_error = nullptr, const FileWriteOptions& a_options = {});
}
