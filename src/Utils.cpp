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

void Utils::drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg, raylib::Color bgLines) {
	const int sides = Attribute::COUNT;
	static raylib::Font font{};
	float baseRotation = 90.0f + 180.0f / sides;
	DrawPoly(center, sides, sideLength, baseRotation, bg);
	DrawPolyLines(center, sides, sideLength-1, baseRotation, WHITE);
	for (int i = 1; i < 5; i++) DrawPolyLines(center, sides, i * sideLength / 5, baseRotation, bgLines);
	for (int i = 0; i < sides; i++) center.DrawLine(center + raylib::Vector2{0, -sideLength}.Rotate(i * 2.0f * PI / sides), bgLines);
	for (int i = 0; i < sides; i++) {
		std::string attrName = (std::string)Attribute{Attribute::Values[i]}.toString().substr(0, 3);
		auto pos = center + raylib::Vector2{0, -(sideLength * 1.2f)}.Rotate(i * 2.0f * PI / sides);
		Utils::drawTextCenteredShadow(attrName, pos, font, 16);
	}
	for (auto [attrs, color, drawNumbers] : attributes) {
		if (attrs.size() != sides) continue;
		std::vector<raylib::Vector2> points;
		for (auto [idx, attrValue] : Utils::enumerate(attrs)) {
			auto [attr, value] = attrValue;
			value = std::clamp(value, 1, 10);
			auto cur = center + raylib::Vector2{0, -(sideLength * value / 10.0f)}.Rotate(idx * 2.0f * PI / sides);
			if (idx > 0) cur.DrawLine(points.back(), color);
			points.push_back(cur);
		}
		points.back().DrawLine(points.front(), color);
		points.push_back(points.front());
		points.push_back(center);
		std::reverse(points.begin(), points.end());
		DrawTriangleFan(points.data(), points.size(), color.Fade(0.5f));
		std::reverse(points.begin(), points.end());
		for (auto [idx, attrValue] : Utils::enumerate(attrs)) {
			auto [attr, value] = attrValue;
			auto point = points[idx];
			if (drawNumbers) {
				point.DrawCircle(6, color);
				auto text = std::to_string(value);
				auto sz = font.MeasureText(text, 12, 2);
				font.DrawText(text, point - sz/2, 12, 2, BLACK);
			} else point.DrawCircle(3, color.Fade(0.4f));
		}
	}
}

void Utils::drawTextCentered(const std::string& text, raylib::Vector2 center, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
	Utils::drawTextCentered(text, center, {}, size, color, spacing, shadow, shadowColor, shadowSpacing);
}
void Utils::drawTextCentered(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
	raylib::Vector2 textsize = font.MeasureText(text, size, spacing);
	if (shadow) font.DrawText(text, center - textsize/2 - raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
	font.DrawText(text, center - textsize/2, size, spacing, color);
}

void Utils::drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, int size, raylib::Color color, int spacing, raylib::Color shadowColor, float shadowSpacing) {
	Utils::drawTextCentered(text, center, {}, size, color, spacing, true, shadowColor, shadowSpacing);
}
void Utils::drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, raylib::Color shadowColor, float shadowSpacing) {
	Utils::drawTextCentered(text, center, font, size, color, spacing, true, shadowColor, shadowSpacing);
}
