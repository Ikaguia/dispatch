#pragma once

namespace Utils {
	std::string toUpper(std::string str);

	namespace Detail {
		constexpr char toLower(char c) {
			return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
		}
	}
	constexpr bool equals(std::string_view a, std::string_view b) {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i) if (Detail::toLower(a[i]) != Detail::toLower(b[i])) return false;
		return true;
	}
}
