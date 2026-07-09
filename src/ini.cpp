#include "ini.hpp"

#include <fstream>

namespace INI {

	Data Read(const VCD::fs::path& a_path)
	{
		Data data;

		std::ifstream file(a_path);
		if (!file.is_open()) {
			return data;
		}

		std::string section;
		std::string line;
		while (std::getline(file, line)) {
			const auto content = StripComment(line);
			if (content.empty()) {
				continue;
			}

			if (TryReadSection(content, section)) {
				data.try_emplace(section);
				continue;
			}

			std::string key;
			std::string value;
			if (!section.empty() && TryReadKeyValue(content, key, value)) {
				data[section][key] = value;
			}
		}

		return data;
	}

}
