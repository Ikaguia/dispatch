#include <string>
#include <locale>
#include <cctype>

#include <Utils.hpp>
#include <algorithm>


std::string Utils::toUpper(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c){ return static_cast<char>(std::toupper(c)); });
	return str;
}
