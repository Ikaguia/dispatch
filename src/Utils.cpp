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

int Utils::randInt(int low, int high) { return rand()%(high-low+1) + low; }

// void Utils::drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg, raylib::Color bgLines, int sides) {
// 	DrawPoly(center, sides, sideLength, 0, bg);
// 	DrawPolyLines(center, sides, sideLength-1, 0, WHITE);
// 	for (int i = 1; i < 5; i++) DrawPolyLines(center, sides, i * sideLength / 5, 0, bgLines);
// 	for (int i = 0; i < sides; i++) center.DrawLine(center + raylib::Vector2{0, -sideLength}.Rotate(i * 360 / sides), bgLines);
// 	for (auto [attrs, color, drawNumbers] : attributes) {
// 		raylib::Vector2 cur, prev, first;
// 		for (auto [idx, attrValue] : Utils::enumerate(attrs)) {
// 			auto [attr, value] = attrValue;
// 			cur = center + raylib::Vector2{0, -(sideLength * value / 10.0f)}.Rotate(idx * 360 / sides);
// 			cur.DrawCircle(5, color.Fade(0.5));
// 			if (idx == 0) first = cur;
// 			else prev.DrawLine(cur, color);
// 			prev = cur;
// 		}
// 		prev.DrawLine(first, color);
// 	}
// }
