#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>

#include "logger.hpp"
#include "helper.hpp"
#include "json_file.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;


// stole from log watcher utiltiy, mabye better to just take the whole header file instead? 

namespace Trans {

    inline fs::path GetTranslationPath() {
        const auto pluginPath = VCD::GetPluginDataPath();
        return pluginPath / "Translation" / "Translation.json";
    }

    class Translator {

        std::unordered_map<std::string, std::string> table;

    public:

        bool load() {

            const auto path = GetTranslationPath();

            table.clear();

            json data;
            if (!JSON::ReadFile(path, data, "Translation")) {
                return false;
            }

            try {
                if (!data.is_object()) {
                    logger::error("Translation: JSON root is not an object in file {}", VCD::ToUTF8(path));
                    return false;
                }

                for (const auto& [k, v] : data.items()) {
                    if (v.is_string()) {
                        table.emplace(k, v.get<std::string>());
                    }
                }
            }
            catch (const std::exception& e) {
                logger::error("Translation: JSON parse error in file {}: {}", VCD::ToUTF8(path), e.what());
                table.clear();
                return false;
            }

            return true;
        }

        const std::string& get(const std::string& key) {
            auto it = table.find(key);
            if (it != table.end()) {
                return it->second;
            }
			return key;
        }

    };

	inline Translator& GetTranslator() {
        static Translator translator;
        return translator;
	}

    inline const std::string& Tr(const std::string& key) {
        return GetTranslator().get(key);
    }

    inline const std::string& Tr(const std::string& key, const int& n) {
		std::string s = Tr(key);
		VCD::ReplaceAll(s, "{n}", std::to_string(n));
		return s;
    }

}
