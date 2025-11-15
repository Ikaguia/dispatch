#include <string>
#include <locale>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <format>

#include <Utils.hpp>
#include <Common.hpp>

std::string Utils::toUpper(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c){ return static_cast<char>(std::toupper(c)); });
	return str;
}

int Utils::randInt(int low, int high) { return rand()%(high-low+1) + low; }

void Utils::drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg, raylib::Color bgLines, bool icons) {
	const int sides = Attribute::COUNT;
	float baseRotation = 90.0f + 180.0f / sides;
	DrawPoly(center, sides, sideLength+1, baseRotation, bg);
	DrawPolyLines(center, sides, sideLength-1, baseRotation, WHITE);
	for (int i = 1; i < 5; i++) DrawPolyLines(center, sides, i * sideLength / 5, baseRotation, bgLines);
	for (int i = 0; i < sides; i++) center.DrawLine(center + raylib::Vector2{0, -sideLength}.Rotate(i * 2.0f * PI / sides), bgLines);
	if (icons) for (int i = 0; i < sides; i++) {
		Attribute attr{i};
		std::string attrIcon{attr.toIcon()};
		auto pos = center + raylib::Vector2{0, -(sideLength * 1.33f)}.Rotate(i * 2.0f * PI / sides);
		pos.DrawCircle(18, bg);
		DrawCircleLines(pos.x, pos.y, 16, WHITE);
		Utils::drawTextCenteredShadow(attrIcon, pos, Dispatch::UI::emojiFont, 26);
		// std::string attrName = (std::string)attr.toString().substr(0, 3);
		// Utils::drawTextCenteredShadow(attrName, pos, Dispatch::UI::defaultFont, 16);
	}
	for (auto [attrs, color, drawNumbers] : attributes) {
		if (attrs.size() != sides) continue;
		std::vector<raylib::Vector2> points;
		for (int i = 0; i < sides; i++) {
			Attribute attr{i};
			int value = attrs[attr];
			value = std::clamp(value, 1, 10);
			auto cur = center + raylib::Vector2{0, -(sideLength * value / 10.0f)}.Rotate(i * 2.0f * PI / sides);
			if (i > 0) cur.DrawLine(points.back(), color);
			points.push_back(cur);
		}
		points.back().DrawLine(points.front(), color);
		points.push_back(points.front());
		points.push_back(center);
		std::reverse(points.begin(), points.end());
		DrawTriangleFan(points.data(), points.size(), color.Fade(0.5f));
		std::reverse(points.begin(), points.end());
		for (int idx = 0; idx < sides; idx++) {
			Attribute attr{idx};
			int value = attrs[attr];
			auto point = points[idx];
			value = std::clamp(value, 1, 10);
			if (drawNumbers) {
				point.DrawCircle(6, color);
				auto text = std::to_string(value);
				auto sz = Dispatch::UI::defaultFont.MeasureText(text, 12, 2);
				Dispatch::UI::defaultFont.DrawText(text, point - sz/2, 12, 2, BLACK);
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

void Utils::drawTextCenteredX(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
	raylib::Vector2 textsize{font.MeasureText(text, size, spacing).x, 0};
	if (shadow) font.DrawText(text, center - textsize/2 - raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
	font.DrawText(text, center - textsize/2, size, spacing, color);
}

void Utils::drawTextCenteredY(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
	raylib::Vector2 textsize{0, font.MeasureText(text, size, spacing).y};
	if (shadow) font.DrawText(text, center - textsize/2 - raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
	font.DrawText(text, center - textsize/2, size, spacing, color);
}

void Utils::drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, int size, raylib::Color color, int spacing, raylib::Color shadowColor, float shadowSpacing) {
	Utils::drawTextCentered(text, center, {}, size, color, spacing, true, shadowColor, shadowSpacing);
}
void Utils::drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, raylib::Color shadowColor, float shadowSpacing) {
	Utils::drawTextCentered(text, center, font, size, color, spacing, true, shadowColor, shadowSpacing);
}

void Utils::drawLineGradient(const raylib::Vector2& src, const raylib::Vector2& dest, raylib::Color srcColor, raylib::Color destColor, int steps) {
	if (steps < 2) throw std::invalid_argument("Steps needs to be >= 2");
	raylib::Vector2 step = (dest-src) / float(steps), cur = src, nxt;
	raylib::Color color;
	for (int i = 0; i < (steps+1); i++) {
		nxt = cur + step;
		color = srcColor.Lerp(destColor, (1.0f*i)/steps);
		DrawLineEx(cur, nxt, 4, color);
		cur = nxt;
	}
}

void Utils::drawFilledCircleVertical(const raylib::Vector2& center, float radius, float filled, raylib::Color topColor, raylib::Color bottomColor, raylib::Color borderColor, float borderThickness) {
	filled = std::clamp(filled, 0.0f, 1.0f);
	if (borderThickness > 0.0f) {
		center.DrawCircle(radius, borderColor);
		radius -= borderThickness;
	}
	center.DrawCircle(radius, topColor);
	float filledHeight = 2.0f * radius * filled;
	raylib::Rectangle clip{center.x - radius, center.y + radius - filledHeight, radius * 2.0f, filledHeight};
	BeginScissorMode(static_cast<int>(clip.x), static_cast<int>(clip.y), static_cast<int>(clip.width), static_cast<int>(clip.height));
		center.DrawCircle(radius, bottomColor);
	EndScissorMode();
}



std::string Utils::addLineBreaks(const std::string_view& text, float maxWidth, const raylib::Font& font, float fontSize, float spacing, const std::string& dividers) {
	std::string out;
	std::string cur(text);

	while (!cur.empty()) {
		std::string sub = cur;
		raylib::Vector2 sz = font.MeasureText(sub, fontSize, spacing);

		// Fits completely
		if (sz.x <= maxWidth) {
			if (!out.empty()) out += '\n';
			out += sub;
			break;
		}

		// Find the largest prefix that fits
		size_t cut = cur.size();
		do {
			size_t lastDivider = cur.find_last_of(dividers, cut - 1);
			if (lastDivider == std::string::npos) {
				// No divider fits — hard break
				cut = std::max<size_t>(1, cut - 1);
				while (cut > 0 && font.MeasureText(cur.substr(0, cut), fontSize, spacing).x > maxWidth)
					--cut;
				break;
			}
			if (font.MeasureText(cur.substr(0, lastDivider), fontSize, spacing).x <= maxWidth) {
				cut = lastDivider;
				break;
			}
			cut = lastDivider;
		} while (cut > 0);

		if (cut == 0) throw std::invalid_argument("Can't fit any text into width " + std::to_string(maxWidth));

		if (!out.empty()) out += '\n';
		out += cur.substr(0, cut);

		// Skip divider for next line
		if (cut < cur.size() && dividers.find(cur[cut]) != std::string::npos) ++cut;

		cur = cur.substr(cut);
	}

	return out;
}


raylib::Vector2 Utils::center(const raylib::Rectangle& rect) {
	return rect.GetPosition() + rect.GetSize()/2;
}

raylib::Rectangle Utils::inset(const raylib::Rectangle& rect, int inset) {
	return {rect.x+inset, rect.y+inset, rect.width-(inset*2), rect.height-(inset*2)};
}

raylib::Rectangle Utils::inset(const raylib::Rectangle& rect, raylib::Vector2 inset) {
	return {rect.x+inset.x, rect.y+inset.y, rect.width-(inset.x*2), rect.height-(inset.y*2)};
}
