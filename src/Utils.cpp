#include <string>
#include <locale>
#include <cctype>
#include <iostream>

#include <Utils.hpp>
#include <algorithm>

std::string Utils::toUpper(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c){ return static_cast<char>(std::toupper(c)); });
	return str;
}

int Utils::randInt(int low, int high) { return rand()%(high-low+1) + low; }

void Utils::drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg, raylib::Color bgLines, int sides) {
	float baseRotation = 90.0f + 180.0f / sides;
	DrawPoly(center, sides, sideLength, baseRotation, bg);
	DrawPolyLines(center, sides, sideLength-1, baseRotation, WHITE);
	for (int i = 1; i < 5; i++) DrawPolyLines(center, sides, i * sideLength / 5, baseRotation, bgLines);
	for (int i = 0; i < sides; i++) center.DrawLine(center + raylib::Vector2{0, -sideLength}.Rotate(i * 2.0f * PI / sides), bgLines);
	for (auto [attrs, color, drawNumbers] : attributes) {
		if (attrs.size() < 3) continue;
		std::vector<raylib::Vector2> points;
		for (auto [idx, attrValue] : Utils::enumerate(attrs)) {
			auto [attr, value] = attrValue;
			value = std::clamp(value, 1, 10);
			auto cur = center + raylib::Vector2{0, -(sideLength * value / 10.0f)}.Rotate(idx * 2.0f * PI / sides);
			cur.DrawCircle(5, color.Fade(0.4f));
			if (idx > 0) cur.DrawLine(points.back(), color);
			points.push_back(cur);
		}
		points.back().DrawLine(points.front(), color);
		points.push_back(points.front());
		points.push_back(center);
		std::reverse(points.begin(), points.end());
		DrawTriangleFan(points.data(), points.size(), color.Fade(0.5f));
	}
}
