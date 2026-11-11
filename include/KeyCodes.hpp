#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <string>
#include <string_view>

namespace KeyNames
{
	struct KeyEntry
	{
		std::string_view name;
		int code;
	};

	inline constexpr std::array kKeys{
		KeyEntry{ "A", 30 }, KeyEntry{ "B", 48 }, KeyEntry{ "C", 46 }, KeyEntry{ "D", 32 },
		KeyEntry{ "E", 18 }, KeyEntry{ "F", 33 }, KeyEntry{ "G", 34 }, KeyEntry{ "H", 35 },
		KeyEntry{ "I", 23 }, KeyEntry{ "J", 36 }, KeyEntry{ "K", 37 }, KeyEntry{ "L", 38 },
		KeyEntry{ "M", 50 }, KeyEntry{ "N", 49 }, KeyEntry{ "O", 24 }, KeyEntry{ "P", 25 },
		KeyEntry{ "Q", 16 }, KeyEntry{ "R", 19 }, KeyEntry{ "S", 31 }, KeyEntry{ "T", 20 },
		KeyEntry{ "U", 22 }, KeyEntry{ "V", 47 }, KeyEntry{ "W", 17 }, KeyEntry{ "X", 45 },
		KeyEntry{ "Y", 21 }, KeyEntry{ "Z", 44 },

		KeyEntry{ "1", 2 }, KeyEntry{ "2", 3 }, KeyEntry{ "3", 4 }, KeyEntry{ "4", 5 },
		KeyEntry{ "5", 6 }, KeyEntry{ "6", 7 }, KeyEntry{ "7", 8 }, KeyEntry{ "8", 9 },
		KeyEntry{ "9", 10 }, KeyEntry{ "0", 11 },

		KeyEntry{ "Minus", 12 }, KeyEntry{ "Equals", 13 },
		KeyEntry{ "Left Bracket", 26 }, KeyEntry{ "Right Bracket", 27 },
		KeyEntry{ "Backslash", 43 }, KeyEntry{ "Semicolon", 39 },
		KeyEntry{ "Apostrophe", 40 }, KeyEntry{ "Grave", 41 },
		KeyEntry{ "Comma", 51 }, KeyEntry{ "Period", 52 }, KeyEntry{ "Slash", 53 },

		KeyEntry{ "F1", 59 }, KeyEntry{ "F2", 60 }, KeyEntry{ "F3", 61 }, KeyEntry{ "F4", 62 },
		KeyEntry{ "F5", 63 }, KeyEntry{ "F6", 64 }, KeyEntry{ "F7", 65 }, KeyEntry{ "F8", 66 },
		KeyEntry{ "F9", 67 }, KeyEntry{ "F10", 68 }, KeyEntry{ "F11", 87 }, KeyEntry{ "F12", 88 },

		KeyEntry{ "Escape", 1 }, KeyEntry{ "Tab", 15 }, KeyEntry{ "Caps Lock", 58 },
		KeyEntry{ "Left Shift", 42 }, KeyEntry{ "Right Shift", 54 },
		KeyEntry{ "Left Ctrl", 29 }, KeyEntry{ "Right Ctrl", 157 },
		KeyEntry{ "Left Alt", 56 }, KeyEntry{ "Right Alt", 184 },
		KeyEntry{ "Space", 57 }, KeyEntry{ "Enter", 28 }, KeyEntry{ "Backspace", 14 },

		KeyEntry{ "Insert", 210 }, KeyEntry{ "Delete", 211 },
		KeyEntry{ "Home", 199 }, KeyEntry{ "End", 207 },
		KeyEntry{ "Page Up", 201 }, KeyEntry{ "Page Down", 209 },
		KeyEntry{ "Up Arrow", 200 }, KeyEntry{ "Down Arrow", 208 },
		KeyEntry{ "Left Arrow", 203 }, KeyEntry{ "Right Arrow", 205 },

		KeyEntry{ "Num Lock", 69 }, KeyEntry{ "Numpad /", 181 }, KeyEntry{ "Numpad *", 55 },
		KeyEntry{ "Numpad -", 74 }, KeyEntry{ "Numpad +", 78 }, KeyEntry{ "Numpad Enter", 156 },
		KeyEntry{ "Numpad .", 83 },
		KeyEntry{ "Numpad 0", 82 }, KeyEntry{ "Numpad 1", 79 }, KeyEntry{ "Numpad 2", 80 },
		KeyEntry{ "Numpad 3", 81 }, KeyEntry{ "Numpad 4", 75 }, KeyEntry{ "Numpad 5", 76 },
		KeyEntry{ "Numpad 6", 77 }, KeyEntry{ "Numpad 7", 71 }, KeyEntry{ "Numpad 8", 72 },
		KeyEntry{ "Numpad 9", 73 }
	};

	// Extra spellings that map onto the same codes.
	inline constexpr std::array kAliases{
		KeyEntry{ "Esc", 1 }, KeyEntry{ "Caps", 58 }, KeyEntry{ "Shift", 42 },
		KeyEntry{ "LShift", 42 }, KeyEntry{ "RShift", 54 },
		KeyEntry{ "Ctrl", 29 }, KeyEntry{ "Control", 29 }, KeyEntry{ "LCtrl", 29 }, KeyEntry{ "RCtrl", 157 },
		KeyEntry{ "Alt", 56 }, KeyEntry{ "LAlt", 56 }, KeyEntry{ "RAlt", 184 },
		KeyEntry{ "Return", 28 }, KeyEntry{ "Ins", 210 }, KeyEntry{ "Del", 211 },
		KeyEntry{ "PgUp", 201 }, KeyEntry{ "PgDn", 209 },
		KeyEntry{ "Up", 200 }, KeyEntry{ "Down", 208 }, KeyEntry{ "Left", 203 }, KeyEntry{ "Right", 205 },
		KeyEntry{ "-", 12 }, KeyEntry{ "=", 13 }, KeyEntry{ "[", 26 }, KeyEntry{ "]", 27 },
		KeyEntry{ "\\", 43 }, KeyEntry{ ";", 39 }, KeyEntry{ "'", 40 }, KeyEntry{ "`", 41 },
		KeyEntry{ ",", 51 }, KeyEntry{ ".", 52 }, KeyEntry{ "/", 53 }
	};

	inline bool IsDigit(unsigned char a_character)
	{
		return std::isdigit(a_character) != 0;
	}

	inline std::string Normalize(std::string_view a_text)
	{
		std::string out;
		out.reserve(a_text.size());
		for (const char c : a_text) {
			if (c == ' ' || c == '_' || c == '\t') {
				continue;
			}
			out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}
		return out;
	}

	inline std::string_view NameFromCode(int a_code)
	{
		for (const auto& key : kKeys) {
			if (key.code == a_code) {
				return key.name;
			}
		}
		return {};
	}

	// Accepts a key name, "None" (-1), or a raw code number (1-255).
	inline bool TryParse(std::string_view a_text, int& a_codeOut)
	{
		const auto text = Normalize(a_text);
		if (text.empty() || text == "none") {
			a_codeOut = -1;
			return true;
		}

		for (const auto& key : kKeys) {
			if (Normalize(key.name) == text) {
				a_codeOut = key.code;
				return true;
			}
		}
		for (const auto& key : kAliases) {
			if (Normalize(key.name) == text) {
				a_codeOut = key.code;
				return true;
			}
		}

		if (std::all_of(text.begin(), text.end(), IsDigit) && text.size() <= 3) {
			const int value = std::stoi(text);
			if (value >= 1 && value <= 255) {
				a_codeOut = value;
				return true;
			}
		}

		return false;
	}
}