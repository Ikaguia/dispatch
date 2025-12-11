#include <string>
#include <locale>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <format>
#include <fstream>
#include <map>

#include <Utils.hpp>
#include <Common.hpp>

using nlohmann::json;

namespace Utils {
	std::string toUpper(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
		return str;
	}

	std::string toLower(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
		return str;
	}

	int randInt(int low, int high) { return rand()%(high-low+1) + low; }

	std::vector<int> range(int start, int end, int step) {
		if (step == 0) throw std::invalid_argument("Step cannot be zero");
		if (step < 0 && start < end) std::swap(start, end);
		std::vector<int> result; result.reserve((std::abs(end - start) + 1) / std::abs(step));
		if (step > 0) for (int i = start; i <= end; i += step) result.push_back(i);
		else for (int i = start; i >= end; i += step) result.push_back(i);
		return result;
	}

	void drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg, raylib::Color bgLines, bool icons) {
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
			drawTextCenteredShadow(attrIcon, pos, Dispatch::UI::emojiFont, 26);
			// std::string attrName = (std::string)attr.toString().substr(0, 3);
			// drawTextCenteredShadow(attrName, pos, Dispatch::UI::defaultFont, 16);
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
				} else if (sideLength > 20) point.DrawCircle(3, color.Fade(0.4f));
			}
		}
	}

	void drawCircularTexture(const raylib::Texture& tex, const raylib::Vector2& pos, float radius, float attenuation, raylib::Vector2 offset) {
		static raylib::Shader circleMaskShader{0, "resources/shaders/circle-mask.fs"};
		static int resolutionUniform = circleMaskShader.GetLocation("resolution");
		static int centerUniform = circleMaskShader.GetLocation("center");
		static int radiusUniform = circleMaskShader.GetLocation("radius");
		static int attenuationUniform = circleMaskShader.GetLocation("attenuation");
		static int texOffsetUniform = circleMaskShader.GetLocation("texOffset");
		float diameter = radius * 2.0f;

		raylib::Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
		raylib::Rectangle dst = {
			pos.x - radius,
			pos.y - radius,
			diameter,
			diameter
		};
		raylib::Vector2 localResolution{diameter, diameter};
		raylib::Vector2 center{radius, radius};

		// Set shader uniforms
		circleMaskShader.SetValue(resolutionUniform, &localResolution, SHADER_UNIFORM_VEC2);
		circleMaskShader.SetValue(centerUniform, &center, SHADER_UNIFORM_VEC2);
		circleMaskShader.SetValue(radiusUniform, &radius, SHADER_UNIFORM_FLOAT);
		circleMaskShader.SetValue(attenuationUniform, &attenuation, SHADER_UNIFORM_FLOAT);
		circleMaskShader.SetValue(texOffsetUniform, &offset, SHADER_UNIFORM_VEC2);

		circleMaskShader.BeginMode();
			DrawTexturePro(tex, src, dst, {0, 0}, 0.0f, WHITE);
		circleMaskShader.EndMode();
	}

	void drawTextureAnchored(const raylib::Texture& tex, raylib::Rectangle dest, FillType fillType, Anchor anchor, AnchorType anchorType) {
		drawTextureAnchored(tex, raylib::Rectangle{0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)}, dest, 0.0f, WHITE, fillType, anchor, anchorType);
	}
	void drawTextureAnchored(const raylib::Texture& tex, raylib::Rectangle dest, raylib::Color color, FillType fillType, Anchor anchor, AnchorType anchorType) {
		drawTextureAnchored(tex, raylib::Rectangle{0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)}, dest, 0.0f, color, fillType, anchor, anchorType);
	}
	void drawTextureAnchored(const raylib::Texture& tex, raylib::Rectangle src, raylib::Rectangle dest, float rotation, raylib::Color color, FillType fillType, Anchor anchor, AnchorType anchorType) {
		raylib::Vector2 srcSize = src.GetSize();
		raylib::Vector2 destSize = dest.GetSize();

		float srcAspect  = srcSize.x / srcSize.y;
		float destAspect = destSize.x / destSize.y;

		switch (fillType) {
			// STRETCH (stretch src to cover dest)
			case FillType::stretch:
				break;
			// FILL (stretch and crop src to cover dest without distortion)
			case FillType::fill: {
				raylib::Vector2 newSrcSize;

				if (srcAspect > destAspect) {
					// too wide → crop width
					newSrcSize.y = srcSize.y;
					newSrcSize.x = destSize.x * (srcSize.y / destSize.y); // srcHeight * destAspect
				} else {
					// too tall → crop height
					newSrcSize.x = srcSize.x;
					newSrcSize.y = destSize.y * (srcSize.x / destSize.x); // srcWidth / destAspect
				}

				// anchorRect returns the new SRC rectangle positioned inside original SRC
				raylib::Rectangle newSrc = anchorRect(src, newSrcSize, anchor, {0,0}, anchorType);

				src = newSrc;
				break;
			}
			// FIT (stretch src to cover at least one dimension of dest without distortion)
			case FillType::fit: {
				raylib::Vector2 newDestSize;

				if (srcAspect > destAspect) {
					// too wide → reduce height
					newDestSize.x = destSize.x;
					newDestSize.y = destSize.x / srcAspect;
				} else {
					// too tall → reduce width
					newDestSize.y = destSize.y;
					newDestSize.x = destSize.y * srcAspect;
				}

				// anchorRect returns a new DEST rect placed inside original DEST
				raylib::Rectangle newDest = anchorRect(dest, newDestSize, anchor, {0,0}, anchorType);

				dest = newDest;
				break;
			}
			case FillType::tile:
			case FillType::tileX:
			case FillType::tileY:
				throw std::logic_error("Texture tiling not implemented");
			default:
				throw std::logic_error("Unknown texture FillType");
		}

		DrawTexturePro(tex, src, dest, {0,0}, rotation, color);
	}


	void drawTextCentered(const std::string& text, raylib::Vector2 center, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
		drawTextCentered(text, center, {}, size, color, spacing, shadow, shadowColor, shadowSpacing);
	}
	void drawTextCentered(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
		raylib::Vector2 textsize = font.MeasureText(text, size, spacing);
		if (shadow) font.DrawText(text, center - textsize/2 - raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
		font.DrawText(text, center - textsize/2, size, spacing, color);
	}

	void drawTextCenteredX(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
		raylib::Vector2 textsize{font.MeasureText(text, size, spacing).x, 0};
		if (shadow) font.DrawText(text, center - textsize/2 - raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
		font.DrawText(text, center - textsize/2, size, spacing, color);
	}

	void drawTextCenteredY(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, bool shadow, raylib::Color shadowColor, float shadowSpacing) {
		raylib::Vector2 textsize{0, font.MeasureText(text, size, spacing).y};
		if (shadow) font.DrawText(text, center - textsize/2 - raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
		font.DrawText(text, center - textsize/2, size, spacing, color);
	}

	void drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, int size, raylib::Color color, int spacing, raylib::Color shadowColor, float shadowSpacing) {
		drawTextCentered(text, center, {}, size, color, spacing, true, shadowColor, shadowSpacing);
	}
	void drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size, raylib::Color color, int spacing, raylib::Color shadowColor, float shadowSpacing) {
		drawTextCentered(text, center, font, size, color, spacing, true, shadowColor, shadowSpacing);
	}

	void drawTextSequence(const std::vector<std::tuple<std::string, raylib::Font&, int, raylib::Color, int, raylib::Color, float>>& texts, raylib::Vector2 position, bool centerX, bool centerY, int text_spacing, bool horizontal) {
		raylib::Vector2 totalSize{0,0};
		std::vector<raylib::Vector2> sizes;
		for (const auto& [text, font, size, color, spacing, shadowColor, shadowSpacing] : texts) {
			raylib::Vector2 sz = font.MeasureText(text, size, spacing);
			sizes.push_back(sz);
			if (horizontal) {
				totalSize.y = std::max(totalSize.y, sz.y);
				totalSize.x += sz.x + text_spacing;
			} else {
				totalSize.x = std::max(totalSize.x, sz.x);
				totalSize.y += sz.y + text_spacing;
			}
		}
		if (!texts.empty()) {
			if (horizontal) totalSize.x -= text_spacing;
			else totalSize.y -= text_spacing;
		}

		raylib::Vector2 drawPos = position;
		if (horizontal && centerX) drawPos.x -= totalSize.x / 2;
		if (!horizontal && centerY) drawPos.y -= totalSize.y / 2;

		for (size_t i = 0; i < texts.size(); i++) {
			const auto& [text, font, size, color, spacing, shadowColor, shadowSpacing] = texts[i];
			raylib::Vector2 sz = sizes[i];
			raylib::Vector2 pos = drawPos;
			if (horizontal && centerY) pos.y -= sz.y / 2;
			if (!horizontal && centerX) pos.x -= sz.x / 2;
			font.DrawText(text, pos + raylib::Vector2{shadowSpacing, shadowSpacing}, size, spacing, shadowColor);
			font.DrawText(text, pos, size, spacing, color);
			if (horizontal) drawPos.x += sz.x + text_spacing;
			else drawPos.y += sz.y + text_spacing;
		}
	}

	raylib::Vector2 anchorPos(raylib::Rectangle rect, Anchor anchor, raylib::Vector2 offset) {
		auto pos = rect.GetPosition();
		// Anchor X
		if (anchor == Anchor::top			|| anchor == Anchor::center	|| anchor == Anchor::bottom)		pos.x += rect.width / 2;
		if (anchor == Anchor::topRight		|| anchor == Anchor::right	|| anchor == Anchor::bottomRight)	pos.x += rect.width;
		// Anchor Y
		if (anchor == Anchor::left			|| anchor == Anchor::center	|| anchor == Anchor::right)			pos.y += rect.height / 2;
		if (anchor == Anchor::bottomLeft	|| anchor == Anchor::bottom	|| anchor == Anchor::bottomRight)	pos.y += rect.height;
		return pos+offset;
	}
	raylib::Rectangle anchorRect(raylib::Rectangle rect, raylib::Vector2 sz, Anchor anchor, raylib::Vector2 offset, AnchorType anchorType) {
		static std::map<Anchor, AnchorType> automatic = {
			{Anchor::topLeft,		AnchorType::bottomRight},
			{Anchor::top,			AnchorType::bottom},
			{Anchor::topRight,		AnchorType::bottomLeft},
			{Anchor::left,			AnchorType::right},
			{Anchor::center,		AnchorType::center},
			{Anchor::right,			AnchorType::left},
			{Anchor::bottomLeft,	AnchorType::topRight},
			{Anchor::bottom,		AnchorType::top},
			{Anchor::bottomRight,	AnchorType::topLeft},
		};
		if (anchorType == AnchorType::automatic) anchorType = automatic[anchor];
		auto pos = anchorPos(rect, anchor, offset);
		// Anchor X
		if (anchorType == AnchorType::top		|| anchorType == AnchorType::center	|| anchorType == AnchorType::bottom)	pos.x -= sz.x / 2;
		if (anchorType == AnchorType::topLeft	|| anchorType == AnchorType::left	|| anchorType == AnchorType::bottomLeft)pos.x -= sz.x;
		// Anchor Y
		if (anchorType == AnchorType::left		|| anchorType == AnchorType::center || anchorType == AnchorType::right)		pos.y -= sz.y / 2;
		if (anchorType == AnchorType::topLeft	|| anchorType == AnchorType::top	|| anchorType == AnchorType::topRight)	pos.y -= sz.y;
		return {pos, sz};
	}
	raylib::Rectangle drawTextAnchored(std::string text, raylib::Rectangle rect, Anchor anchor, const raylib::Font& font, raylib::Color color, float size, float spacing, raylib::Vector2 offset, float maxW, AnchorType anchorType) {
		auto sz = font.MeasureText(text, size, spacing);
		if (maxW != -1 && sz.x > maxW) {
			text = addLineBreaks(text, maxW, font, size, spacing);
			sz = font.MeasureText(text, size, spacing);
		}
		auto drawRect = anchorRect(rect, sz, anchor, offset, anchorType);
		font.DrawText(text, drawRect.GetPosition(), size, spacing, color);
		return drawRect;
	}
	raylib::Rectangle positionTextAnchored(std::string text, raylib::Rectangle rect, Anchor anchor, const raylib::Font& font, float size, float spacing, raylib::Vector2 offset, float maxW, AnchorType anchorType) {
		auto sz = font.MeasureText(text, size, spacing);
		if (maxW != -1 && sz.x > maxW) {
			text = addLineBreaks(text, maxW, font, size, spacing);
			sz = font.MeasureText(text, size, spacing);
		}
		return anchorRect(rect, sz, anchor, offset, anchorType);
	}

	std::vector<raylib::Rectangle> splitRect(raylib::Rectangle rect, int rows, int cols, raylib::Vector2 divider) {
		std::vector<raylib::Rectangle> out;
		raylib::Vector2 size = {(rect.width - divider.x*(cols-1)) / cols, (rect.height - divider.y*(rows-1)) / rows};
		for (int row = 0; row < rows; row++) {
			auto pos = rect.GetPosition() + raylib::Vector2{0.0f, (size.y+divider.y)*row};
			for (int col = 0; col < cols; col++) {
				out.emplace_back(pos, size);
				pos.x += size.x + divider.x;
			}
		}
		return out;
	}

	void drawLineGradient(const raylib::Vector2& src, const raylib::Vector2& dest, raylib::Color srcColor, raylib::Color destColor, int steps) {
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

	void drawFilledCircleVertical(const raylib::Vector2& center, float radius, float filled, raylib::Color topColor, raylib::Color bottomColor, raylib::Color borderColor, float borderThickness) {
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



	std::string addLineBreaks(const std::string_view& text, float maxWidth, const raylib::Font& font, float fontSize, float spacing, const std::string& dividers) {
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


	raylib::Vector2 center(const raylib::Rectangle& rect) {
		return rect.GetPosition() + rect.GetSize()/2;
	}

	raylib::Rectangle inset(const raylib::Rectangle& rect, int inset) {
		return {rect.x+inset, rect.y+inset, rect.width-(inset*2), rect.height-(inset*2)};
	}

	raylib::Rectangle inset(const raylib::Rectangle& rect, raylib::Vector2 inset) {
		return {rect.x+inset.x, rect.y+inset.y, rect.width-(inset.x*2), rect.height-(inset.y*2)};
	}

	std::string readFile(std::string path) {
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		if (!file.is_open()) throw std::runtime_error("Failed to open file: " + path);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::string buffer(size, '\0');
		file.read(buffer.data(), size);
		return buffer;
	}

	json readJsonFile(std::string path) {
		std::string buffer = readFile(path);
		return json::parse(buffer);
	}
};
