#pragma once

#include <utility>
#include <cstddef>
#include <iterator>
#include <random>
#include <iostream>
#include <format>

#include <raylib-cpp.hpp>
#include <Attribute.hpp>

namespace Utils {
	std::string toUpper(std::string str);

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

	void drawRadarGraph(raylib::Vector2 center, float sideLength, std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attributes, raylib::Color bg = BLACK, raylib::Color bgLines = BROWN, bool icons=true);

	void drawTextCentered(const std::string& text, raylib::Vector2 center, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCentered(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredX(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredY(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, bool shadow=false, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, int size=12, raylib::Color color=WHITE, int spacing=2, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);
	void drawTextCenteredShadow(const std::string& text, raylib::Vector2 center, const raylib::Font& font, int size=12, raylib::Color color=WHITE, int spacing=2, raylib::Color shadowColor=BLACK, float shadowSpacing=1.5f);

	void drawTextSequence(const std::vector<std::tuple<std::string, raylib::Font&, int, raylib::Color, int, raylib::Color, float>>& text, raylib::Vector2 position, bool centerX=false, bool centerY=false, int text_spacing=0, bool horizontal=true);

	enum struct Anchor { topLeft, top, topRight, left, center, right, bottomLeft, bottom, bottomRight };
	enum struct AnchorType { automatic, topLeft, top, topRight, left, center, right, bottomLeft, bottom, bottomRight };
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

	std::string readFile(std::string path);
}
