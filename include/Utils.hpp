#pragma once

#include <utility>
#include <cstddef>
#include <iterator>
#include <random>
#include <iostream>
#include <format>

#include <raylib-cpp.hpp>
#include <nlohmann/json.hpp>
#include <Attribute.hpp>
#include <Common.hpp>

namespace Utils {
	#define BEGEND(i) (i).begin(), (i).end()

	template<typename T>
	using optRef = std::optional<std::reference_wrapper<T>>;

	enum struct Anchor { topLeft, top, topRight, left, center, right, bottomLeft, bottom, bottomRight };
	enum struct AnchorType { automatic, topLeft, top, topRight, left, center, right, bottomLeft, bottom, bottomRight };
	enum struct FillType { stretch, tile, tileX, tileY, fit, fill };

	std::string toUpper(std::string str);
	std::string toLower(std::string str);

	std::string escapeRegex(const std::string &text);
	std::vector<std::string> split(const std::string& str, const std::string& sep);

	void replaceAll(std::string& target, const std::string& toReplace, const std::string& replacement);

	int randInt(int low, int high);
	std::vector<int> range(int start, int end, int step=1);

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
	template<typename... Args>
	constexpr bool equals(std::string_view a, std::string_view b, Args... args) { return ((equals(a, b) || ... || equals(a, args))); }

	template <typename T>
	std::string join(const T& container, const std::string& delim) {
		std::ostringstream oss;
		auto it = container.begin();
		if (it != container.end()) oss << *it++;
		while (it != container.end()) oss << delim << *it++;
		return oss.str();
	}

	template <typename T>
	T clamp(T val, T mn, T mx) { return val < mn ? mn : val > mx ? mx : val; }

	template <typename T>
	constexpr auto enumerate(T&& iterable) {
		struct iterator {
			size_t i;
			decltype(std::begin(iterable)) iter;

			bool operator!=(const iterator& other) const { return iter != other.iter; }
			void operator++() { ++i; ++iter; }
			auto operator*() const { return std::tie(i, *iter); }
		};

		struct iterable_wrapper {
			T iterable;
			auto begin() { return iterator{0, std::begin(iterable)}; }
			auto end() { return iterator{0, std::end(iterable)}; }
		};

		return iterable_wrapper{std::forward<T>(iterable)};
	}

	template <typename Set>
	auto random_element(const Set& s) -> const typename Set::value_type& {
		static std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<size_t> dist(0, s.size() - 1);
		return *std::next(s.begin(), dist(rng));
	}

	void drawCircularTexture(const raylib::Texture& tex, const raylib::Vector2& pos, float radius, float attenuation=0.0f, raylib::Vector2 offset={0.0f, 0.0f});

	void drawTextureAnchored(const raylib::Texture& tex, raylib::Rectangle dest, FillType fillType=FillType::fill, Anchor anchor = Anchor::center, AnchorType anchorType = AnchorType::automatic);
	void drawTextureAnchored(const raylib::Texture& tex, raylib::Rectangle dest, raylib::Color color, FillType fillType=FillType::fill, Anchor anchor = Anchor::center, AnchorType anchorType = AnchorType::automatic);
	void drawTextureAnchored(const raylib::Texture& tex, raylib::Rectangle src, raylib::Rectangle dest, float rotation=0.0f, raylib::Color color = WHITE, FillType fillType=FillType::fill, Anchor anchor = Anchor::center, AnchorType anchorType = AnchorType::automatic);

	void drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg = BLACK, raylib::Color bgLines = BROWN, bool icons=true);

	void drawTextCentered(const std::string& text, raylib::Vector2 center, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCentered(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredX(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredY(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, int size=12, raylib::Color color=WHITE, int spacing=2, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);

	void drawTextSequence(const std::vector<std::tuple<std::string, raylib::Font&, int, raylib::Color, int, raylib::Color, float>>& text, raylib::Vector2 position, bool centerX=false, bool centerY=false, int text_spacing=0, bool horizontal=true);

	raylib::Vector2 anchorPos(raylib::Rectangle rect, Anchor anchor, raylib::Vector2 offset = {0.0f, 0.0f});
	raylib::Rectangle anchorRect(raylib::Rectangle rect, raylib::Vector2 sz, Anchor anchor, raylib::Vector2 offset = {0.0f, 0.0f}, AnchorType anchorType = AnchorType::automatic);
	raylib::Rectangle drawTextAnchored(std::string text, raylib::Rectangle rect, Anchor anchor, const raylib::Font& font, raylib::Color color=WHITE, float size=12, float spacing=2, raylib::Vector2 offset={0.0f, 0.0f}, float maxW=-1, AnchorType anchorType = AnchorType::automatic);
	raylib::Rectangle positionTextAnchored(std::string text, raylib::Rectangle rect, Anchor anchor, const raylib::Font& font, float size=12, float spacing=2, raylib::Vector2 offset={0.0f, 0.0f}, float maxW=-1, AnchorType anchorType = AnchorType::automatic);

	std::vector<raylib::Rectangle> splitRect(raylib::Rectangle rect, int rows, int cols, raylib::Vector2 divider);

	void drawLineGradient(const raylib::Vector2& src, const raylib::Vector2& dest, raylib::Color srcColor, raylib::Color destColor, int steps=50);

	void drawFilledCircleVertical(const raylib::Vector2& center, float radius, float filled, raylib::Color topColor, raylib::Color bottomColor, raylib::Color borderColor=BLACK, float borderThickness=1.0f);

	std::string addLineBreaks(const std::string_view& text, float maxWidth, const raylib::Font& font, float fontSize=12, float spacing=2, const std::string& dividers=" ");

	raylib::Vector2 center(const raylib::Rectangle& rect);
	raylib::Rectangle inset(const raylib::Rectangle& rect, int inset);
	raylib::Rectangle inset(const raylib::Rectangle& rect, raylib::Vector2 inset);

	template <typename... Args>
	inline void print(std::string_view fmt, Args&&... args) {
		std::cout << std::vformat(fmt, std::make_format_args(args...));
	}

	template <typename... Args>
	inline void println(std::string_view fmt, Args&&... args) {
		std::cout << std::vformat(fmt, std::make_format_args(args...)) << '\n';
	}

	inline void println(std::string_view text) { std::cout << text << '\n'; }

	std::vector<std::string> getFilesInFolder(const std::string& path, const std::string& extension="");

	std::string readFile(std::string path);

	nlohmann::json readJsonFile(std::string path);
}

namespace nlohmann {
	template <>
	struct adl_serializer<raylib::Vector2> {
		static void to_json(json& j, const raylib::Vector2& v2) { j = json{
			{"x", v2.x},
			{"y", v2.y}
		}; }
		static void from_json(const json& j, raylib::Vector2& v2) {
			if (j.is_object()) {
				v2.x = j.at("x").get<float>();
				v2.y = j.at("y").get<float>();
			} else if (j.is_array()) {
				const auto& jA = j.get<std::vector<float>>();
				if (jA.size() != 2) throw std::runtime_error("Invalid number of elements for Vector2");
				v2.x = jA[0];
				v2.y = jA[1];
			} else if (j.is_number()) {
				v2.x = v2.y = j.get<float>();
			} else throw std::runtime_error("Invalid format for Vector2");
		}
	};
	template <>
	struct adl_serializer<raylib::Vector3> {
		static void to_json(json& j, const raylib::Vector3& v3) { j = json{
			{"x", v3.x},
			{"y", v3.y},
			{"z", v3.z}
		}; }
		static void from_json(const json& j, raylib::Vector3& v3) {
			if (j.is_object()) {
				v3.x = j.at("x").get<float>();
				v3.y = j.at("y").get<float>();
				v3.z = j.at("z").get<float>();
			} else if (j.is_array()) {
				const auto& jA = j.get<std::vector<float>>();
				if (jA.size() != 3) throw std::runtime_error("Invalid number of elements for Vector3");
				v3.x = jA[0];
				v3.y = jA[1];
				v3.z = jA[2];
			} else if (j.is_number()) {
				v3.x = v3.y = v3.z = j.get<float>();
			} else throw std::runtime_error("Invalid format for Vector3");
		}
	};
	template <>
	struct adl_serializer<raylib::Vector4> {
		static void to_json(json& j, const raylib::Vector4& v4) { j = json{
			{"x", v4.x},
			{"y", v4.y},
			{"z", v4.z},
			{"w", v4.w}
		}; }
		static void from_json(const json& j, raylib::Vector4& v4) {
			if (j.is_object()) {
				v4.x = j.at("x").get<float>();
				v4.y = j.at("y").get<float>();
				v4.z = j.at("z").get<float>();
				v4.w = j.at("w").get<float>();
			} else if (j.is_array()) {
				const auto& jA = j.get<std::vector<float>>();
				if (jA.size() != 4) throw std::runtime_error("Invalid number of elements for Vector4");
				v4.x = jA[0];
				v4.y = jA[1];
				v4.z = jA[2];
				v4.w = jA[3];
			} else if (j.is_number()) {
				v4.x = v4.y = v4.z = v4.w = j.get<float>();
			} else throw std::runtime_error("Invalid format for Vector4");
		}
	};
	template <>
	struct adl_serializer<raylib::Rectangle> {
		static void to_json(json& j, const raylib::Rectangle& rec) { j = json{
			{"x", rec.x},
			{"y", rec.y},
			{"width", rec.width},
			{"height", rec.height}
		}; }
		static void from_json(const json& j, raylib::Rectangle& rec) {
			rec.x = j.at("x").get<float>();
			rec.y = j.at("y").get<float>();
			rec.width = j.at("width").get<float>();
			rec.height = j.at("height").get<float>();
		}
	};
	template <>
	struct adl_serializer<raylib::Color> {
		static void to_json(json& j, const raylib::Color& color) { j = json{
			{"r", color.r},
			{"g", color.g},
			{"b", color.b},
			{"a", color.a}
		}; }
		static void from_json(const json& j, raylib::Color& color) {
			if (j.is_object()) {
				if (j.contains("func")) {
					std::string func = j["func"].get<std::string>();
					const json& args = j["args"];
					if (func == "lerp") {
						if (args.is_array() && args.size() == 3 && args[2].is_number_float()) {
							color = ColorLerp(args[0].get<raylib::Color>(), args[1].get<raylib::Color>(), args[2].get<float>());
						} else throw std::runtime_error("Invalid arguments for ColorLerp function: " + j.dump());
					} else if (func == "alpha") {
						if (args.is_array() && args.size() == 2 && args[1].is_number_float()) {
							color = ColorAlpha(args[0].get<raylib::Color>(), args[1].get<float>());
						} else throw std::runtime_error("Invalid arguments for ColorLerp function: " + j.dump());
					} else throw std::runtime_error("Invalid color function: " + j.dump());
				} else {
					color.r = j.at("r").get<unsigned char>();
					color.g = j.at("g").get<unsigned char>();
					color.b = j.at("b").get<unsigned char>();
					if (j.contains("a")) color.a = j.at("a").get<unsigned char>();
					else color.a = 255;
				}
			} else if (j.is_array()) {
				const auto& jA = j.get<std::vector<unsigned char>>();
				if (jA.size() != 4) throw std::runtime_error("Invalid number of elements for Color");
				color.r = jA[0];
				color.g = jA[1];
				color.b = jA[2];
				color.a = jA[3];
			} else if (j.is_string()) {
				std::string colorS = j.get<std::string>();
				if (colorS == "LIGHTGRAY") color = LIGHTGRAY;
				else if (colorS == "GRAY") color = GRAY;
				else if (colorS == "DARKGRAY") color = DARKGRAY;
				else if (colorS == "YELLOW") color = YELLOW;
				else if (colorS == "GOLD") color = GOLD;
				else if (colorS == "ORANGE") color = ORANGE;
				else if (colorS == "PINK") color = PINK;
				else if (colorS == "RED") color = RED;
				else if (colorS == "MAROON") color = MAROON;
				else if (colorS == "GREEN") color = GREEN;
				else if (colorS == "LIME") color = LIME;
				else if (colorS == "DARKGREEN") color = DARKGREEN;
				else if (colorS == "SKYBLUE") color = SKYBLUE;
				else if (colorS == "BLUE") color = BLUE;
				else if (colorS == "DARKBLUE") color = DARKBLUE;
				else if (colorS == "PURPLE") color = PURPLE;
				else if (colorS == "VIOLET") color = VIOLET;
				else if (colorS == "DARKPURPLE") color = DARKPURPLE;
				else if (colorS == "BEIGE") color = BEIGE;
				else if (colorS == "BROWN") color = BROWN;
				else if (colorS == "DARKBROWN") color = DARKBROWN;
				else if (colorS == "WHITE") color = WHITE;
				else if (colorS == "BLACK") color = BLACK;
				else if (colorS == "BLANK") color = BLANK;
				else if (colorS == "MAGENTA") color = MAGENTA;
				else if (colorS == "RAYWHITE") color = RAYWHITE;
				else if (colorS == "BGLGT") color = Dispatch::UI::bgLgt;
				else if (colorS == "BGMED") color = Dispatch::UI::bgMed;
				else if (colorS == "BGDRK") color = Dispatch::UI::bgDrk;
				else if (colorS == "TEXTCOLOR") color = Dispatch::UI::textColor;
				else if (colorS == "SHADOW") color = Dispatch::UI::shadow;
				else throw std::runtime_error("Invalid color name: " + colorS);
			} else throw std::runtime_error("Invalid color format");
		}
	};

	NLOHMANN_JSON_SERIALIZE_ENUM( Utils::Anchor, {
		{Utils::Anchor::center, "center"},
		{Utils::Anchor::topLeft, "topLeft"},
		{Utils::Anchor::top, "top"},
		{Utils::Anchor::topRight, "topRight"},
		{Utils::Anchor::left, "left"},
		{Utils::Anchor::right, "right"},
		{Utils::Anchor::bottomLeft, "bottomLeft"},
		{Utils::Anchor::bottom, "bottom"},
		{Utils::Anchor::bottomRight, "bottomRight"},
	});
	NLOHMANN_JSON_SERIALIZE_ENUM( Utils::AnchorType, {
		{Utils::AnchorType::automatic, "automatic"},
		{Utils::AnchorType::topLeft, "topLeft"},
		{Utils::AnchorType::top, "top"},
		{Utils::AnchorType::topRight, "topRight"},
		{Utils::AnchorType::left, "left"},
		{Utils::AnchorType::center, "center"},
		{Utils::AnchorType::right, "right"},
		{Utils::AnchorType::bottomLeft, "bottomLeft"},
		{Utils::AnchorType::bottom, "bottom"},
		{Utils::AnchorType::bottomRight, "bottomRight"},
	});
	NLOHMANN_JSON_SERIALIZE_ENUM( Utils::FillType, {
		{Utils::FillType::stretch, "stretch"},
		{Utils::FillType::tile, "tile"},
		{Utils::FillType::tileX, "tileX"},
		{Utils::FillType::tileY, "tileY"},
		{Utils::FillType::fit, "fit"},
		{Utils::FillType::fill, "fill"},
	});
};
