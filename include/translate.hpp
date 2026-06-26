#pragma once

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>

#include "logger.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace Utils{


// stole from log watcher utiltiy, mabye better to just take the whole header file instead? 
inline std::string toUTF8(const std::filesystem::path& p) {
    auto u8 = p.u8string();
    return std::string(u8.begin(), u8.end());
}

inline void replaceAll(std::string& s, const std::string& from, const std::string& to) {
    if (from.empty()) return;

    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }
}

}

namespace Trans {

    inline std::string GetTranslationPath() {
        const auto root = std::filesystem::path(REL::Module::get().filename()).parent_path();
        return (root / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / "Translation" / (std::string(PRODUCT_NAME) + "Translation.json")).string();
    }

    class Translator {

        std::unordered_map<std::string, std::string> table;

    public:

        bool load() {

            const auto path = GetTranslationPath();

            table.clear();

            std::ifstream in(path);
            if (!in) {
                logger::error("Translation: could not open file {}", Utils::toUTF8(path));
                return false;
            }

            try {
                json data = json::parse(in, nullptr, true, true);

                if (!data.is_object()) {
                    logger::error("Translation: JSON root is not an object in file {}", Utils::toUTF8(path));
                    return false;
                }

                for (const auto& [k, v] : data.items()) {
                    if (v.is_string()) {
                        table.emplace(k, v.get<std::string>());
                    }
                }
            }
            catch (const std::exception& e) {
                logger::error("Translation: JSON parse error in file {}: {}", Utils::toUTF8(path), e.what());
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
		Utils::replaceAll(s, "{n}", std::to_string(n));
		return s;
    }

}
