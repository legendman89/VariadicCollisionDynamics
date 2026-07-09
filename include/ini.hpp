#pragma once

#include "helper.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace INI {

	using Section = std::unordered_map<std::string, std::string>;
	using Data = std::unordered_map<std::string, Section>;

	inline constexpr auto kListDelimiter = ',';

	inline std::string StripComment(std::string_view a_line)
	{
		const auto comment = a_line.find_first_of(";#");
		return VCD::Trim(comment == std::string_view::npos ? a_line : a_line.substr(0, comment));
	}

	inline bool TryReadSection(std::string_view a_line, std::string& a_section)
	{
		if (a_line.size() < 3 || a_line.front() != '[' || a_line.back() != ']') {
			return false;
		}

		a_section = VCD::Trim(a_line.substr(1, a_line.size() - 2));
		return !a_section.empty();
	}

	inline bool TryReadKeyValue(std::string_view a_line, std::string& a_key, std::string& a_value)
	{
		const auto delimiter = a_line.find('=');
		if (delimiter == std::string_view::npos) {
			return false;
		}

		a_key = VCD::Trim(a_line.substr(0, delimiter));
		a_value = VCD::Trim(a_line.substr(delimiter + 1));
		return !a_key.empty();
	}

	inline const std::string* FindValue(const Data& a_ini, const char* a_section, const char* a_key)
	{
		const auto sectionIt = a_ini.find(a_section);
		if (sectionIt == a_ini.end()) {
			return nullptr;
		}

		const auto valueIt = sectionIt->second.find(a_key);
		if (valueIt == sectionIt->second.end()) {
			return nullptr;
		}

		return &valueIt->second;
	}

	inline void NormalizeValues(std::vector<std::string>& a_values)
	{
		for (auto& value : a_values) {
			value = VCD::Trim(value);
		}

		a_values.erase(std::remove_if(a_values.begin(), a_values.end(), [](const std::string& a_value) { return a_value.empty(); }), a_values.end());
	}

	inline std::vector<std::string> SplitValue(std::string_view a_value, const char& a_delimiter = kListDelimiter)
	{
		std::vector<std::string> values;
		std::stringstream stream{ std::string(a_value) };
		std::string entry;

		while (std::getline(stream, entry, a_delimiter)) {
			values.push_back(VCD::Trim(entry));
		}

		NormalizeValues(values);
		return values;
	}

	Data Read(const VCD::fs::path& a_path);

}
