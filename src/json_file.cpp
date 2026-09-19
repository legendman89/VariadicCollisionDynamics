#include "json_file.hpp"
#include "helper.hpp"
#include "logger.hpp"

#include <Windows.h>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <system_error>

namespace JSON {

    bool ReadFile(const std::filesystem::path& a_path, nlohmann::json& a_data, const char* a_label, bool a_allowComments)
    {
        try {
            std::ifstream file(a_path);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open the file for reading.");
            }

            const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (file.bad()) {
                throw std::runtime_error("Could not read the file.");
            }

            auto data = nlohmann::json::parse(text, nullptr, true, a_allowComments);

            a_data = std::move(data);

            logger::info("{} loaded: {}", a_label, VCD::ToUTF8(a_path));

            return true;
        }
        catch (const std::exception& error) {
            logger::error("Could not read {} file {}: {}", a_label, VCD::ToUTF8(a_path), error.what());
            return false;
        }
    }

    bool WriteFile(const std::filesystem::path& a_path, const nlohmann::json& a_data, const char* a_label, std::string* a_error, const FileWriteOptions& a_options)
    {
        static std::mutex writeMutex;
        const std::lock_guard lock(writeMutex);

        if (a_error) {
            a_error->clear();
        }

        auto temporaryPath = a_path;
        temporaryPath += ".tmp";
        bool temporaryCreated = false;

        try {
            auto text = a_data.dump(a_options.indent);
            if (a_options.trailingNewline) {
                text += '\n';
            }

            if (!a_path.parent_path().empty()) {
                std::filesystem::create_directories(a_path.parent_path());
            }

            std::ofstream file;
            file.exceptions(std::ios::failbit | std::ios::badbit);
            file.open(temporaryPath, std::ios::out | std::ios::trunc);
            temporaryCreated = true;
            file << text;
            file.close();

            if (!MoveFileExW(temporaryPath.c_str(), a_path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Could not replace the JSON file");
            }

            logger::info("{} saved: {}", a_label, VCD::ToUTF8(a_path));

            return true;
        }
        catch (const std::exception& error) {
            if (temporaryCreated) {
                std::error_code cleanupError;
                std::filesystem::remove(temporaryPath, cleanupError);
            }

            if (a_error) {
                *a_error = "Could not save the file. Check the VCD log.";
            }

            logger::error("Could not save {} file {}: {}", a_label, VCD::ToUTF8(a_path), error.what());

            return false;
        }
    }
}
