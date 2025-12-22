#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <string>

#include <raylib-cpp.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <Utils.hpp>
#include <Common.hpp>

namespace Dispatch::UI {
	class Element;

	class Layout {
	public:
		std::map<std::string, std::unique_ptr<Element>> elements;
		std::set<std::string> rootElements, hovered, clicked, pressed, unhover, release, dataChanged;
		std::map<std::string, nlohmann::json> sharedData;
		std::map<std::string, std::set<std::string>> sharedDataListeners;

		Layout(std::string path);

		void render();
		void handleInput();
		void updateSharedData(const std::string& key, const nlohmann::json& value);
		void deleteSharedData(const std::string& key);
		void registerSharedDataListener(const std::string& key, const std::string& element_id);
	};

	enum struct FillType {
		NONE,
		FULL,
		GRADIENT,
		GRADIENT_H,
		GRADIENT_V,
	};

	enum struct Side {
		TOP,
		BOTTOM,
		START,
		END,
		LEFT = START,
		RIGHT = END,
		INVALID = -1
	};

	class Element {
	protected:
		nlohmann::json orig;
		std::set<std::string> dynamic_vars;
	public:
		Layout* layout = nullptr;
		bool visible = true, hovered = false, clicked = false, pressed = false, unhover = false, release = false;
		int z_order = -1, roundnessSegments = 0;
		float borderThickness = 0.0f, roundness = 0.0f;
		std::string id, father_id;
		std::vector<std::string> subElement_ids;
		raylib::Vector2 size, shadowOffset;
		raylib::Vector4 outterMargin, innerMargin;
		raylib::Color innerColor{0,0,0,0}, outterColor{0,0,0,0}, borderColor{0,0,0,0}, shadowColor = ColorAlpha(BLACK, 0.3f);
		struct Constraint {
			float offset = 0.0f, ratio = 0.5f;
			struct ConstraintPart {
				enum ConstraintType { UNATTACHED, FATHER, ELEMENT, SCREEN } type = UNATTACHED;
				std::string element_id;
				Side side;
			} start, end;
		} verticalConstraint, horizontalConstraint;
		enum Status {
			REGULAR,
			HOVERED,
			PRESSED,
			SELECTED,
			DISABLED,
			COUNT
		} status{Status::REGULAR};
		inline static constexpr Status statuses[COUNT] = {Status::REGULAR, Status::HOVERED, Status::PRESSED, Status::SELECTED, Status::DISABLED};

		virtual ~Element() = default;

		virtual const float side(Side side, bool inner=false) const;
		virtual raylib::Vector2 center() const;
		virtual raylib::Vector2 anchor(Utils::Anchor anchor = Utils::Anchor::center) const;
		virtual const raylib::Rectangle& boundingRect() const;
		virtual const raylib::Rectangle& subElementsRect() const;

		virtual const bool colidesWith(const Element& other) const;
		virtual const bool colidesWith(const raylib::Vector2& other) const;
		virtual const bool colidesWith(const raylib::Rectangle& other) const;

		virtual void render();
		virtual void _render();
		virtual void handleInput(raylib::Vector2 offset={});
		virtual void _handleInput(raylib::Vector2 offset={});
		virtual void solveLayout();
		virtual void solveSize();
		virtual void sortSubElements(bool z_order);
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) {}
		virtual void changeStatus(Status st, bool force=false);

		virtual void to_json(nlohmann::json& j) const;
		virtual void from_json(const nlohmann::json& j);
	protected:
		raylib::Rectangle bounds, innerBounds;
		bool initialized = false;
	};

	class Box : public virtual Element {
	public:
		Box() {
			borderThickness = 1.0f;
			innerColor = bgLgt;
			outterColor = bgMed;
			borderColor = BLACK;
		}
	};

	class Text : public virtual Element {
		raylib::Font font = GetFontDefault();
	public:
		int fontSize = 16;
		float spacing = 1.0f;
		std::string text, fontName;
		raylib::Color fontColor = textColor;
		Utils::Anchor textAnchor = Utils::Anchor::center;

		virtual void _render() override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;

		void parseText();
		void updateText();
	};

	class TextBox : public virtual Box, public virtual Text {};

	class Button : public virtual TextBox {
	public:
		struct StatusChanges {
			raylib::Color inner, outter, border, text;
			float size_mult;
			StatusChanges(raylib::Color i, raylib::Color o, raylib::Color b, raylib::Color t, float s) : inner{i}, outter{o}, border{b}, text{t}, size_mult{s} {}
			StatusChanges() = default;
		};
		std::map<Status, StatusChanges> statusChanges;
		float size_mult = 1.0f;

		Button() : TextBox() {
			statusChanges = std::map<Status, StatusChanges>{
				{Status::REGULAR, {bgLgt, bgMed, BLACK, textColor, 1.0f}},
				{Status::HOVERED, {SKYBLUE, BLUE, BLACK, textColor, 1.1f}},
				{Status::PRESSED, {BLUE, DARKBLUE, BLACK, textColor, 1.1f}},
				{Status::SELECTED, {ORANGE, BROWN, BLACK, WHITE, 1.0f}},
				{Status::DISABLED, {bgDrk, DARKGRAY, BLACK, LIGHTGRAY, 1.0f}}
			};
		}

		virtual void solveSize() override;
		virtual void changeStatus(Status st, bool force=false) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void from_json(const nlohmann::json& j) override;
	};

	class RadioButton : public virtual Button {
	public:
		std::string key;

		virtual void _handleInput(raylib::Vector2 offset={}) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
	};

	class Circle : public virtual Box {
	public:
		Circle() : Element(), Box() {
			roundness = 1.0f;
			roundnessSegments = 32;
		}

		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
	};

	class TextCircle : public virtual Circle, public virtual Text {
	public:
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
	};

	class Image : public virtual Element {
	public:
		raylib::Texture texture;
		std::string path;
		Utils::FillType fillType = Utils::FillType::fill;
		Utils::Anchor imageAnchor = Utils::Anchor::center;
		raylib::Color tintColor = WHITE;

		virtual void _render() override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
	};

	class RadarGraph : public virtual Element {
	public:
		struct Segment {
			std::string label, icon;
		};
		struct Group {
			std::vector<float> values;
			raylib::Color color = BLUE;
		};
		std::vector<Segment> segments;
		std::vector<Group> groups;
		float maxValue = 10.0f;
		int fontSize = 14, precision=0;
		raylib::Color fontColor=textColor, overlapColor=WHITE, bgLines=bgDrk;
		bool drawLabels=true, drawIcons=false, drawOverlap=true;

		virtual void _render() override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
	};

	class AttrGraph : public virtual RadarGraph {
	public:
		AttrGraph() : Element(), RadarGraph() {
			for (const Attribute attr : Attribute::Values) segments.push_back({std::string(attr.toString()), std::string(attr.toIcon())});
			innerColor = BLACK;
			borderColor = WHITE;
			drawIcons = true;
		}
	};

	class ScrollBox : public virtual Box {
	public:
		raylib::Vector2 contentSize, scrollOffset;
		bool scrollX=false, scrollY=true;

		virtual void render() override;
		virtual void handleInput(raylib::Vector2 offset={}) override;
		virtual void _handleInput(raylib::Vector2 offset={}) override;
		virtual void solveLayout() override;

		virtual void to_json(nlohmann::json& j) const override;
		virtual void from_json(const nlohmann::json& j) override;
	};
}

namespace nlohmann {
	// JSON Serialization
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Element& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Box& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Box& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Text& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Text& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::TextBox& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::TextBox& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Button& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Button& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::RadioButton& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::RadioButton& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Circle& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Circle& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::TextCircle& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::TextCircle& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Image& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Image& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::RadarGraph& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::RadarGraph& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::AttrGraph& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::AttrGraph& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::ScrollBox& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::ScrollBox& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint& inst);
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint::ConstraintPart& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint::ConstraintPart& inst);
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Button::StatusChanges& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Button::StatusChanges& inst);
	inline void to_json(nlohmann::json& j, const Dispatch::UI::RadarGraph::Segment& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::RadarGraph::Segment& inst);
	inline void to_json(nlohmann::json& j, const Dispatch::UI::RadarGraph::Group& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::RadarGraph::Group& inst);

	NLOHMANN_JSON_SERIALIZE_ENUM( Dispatch::UI::Side, {
		{Dispatch::UI::Side::INVALID, nullptr},
		{Dispatch::UI::Side::TOP, "top"},
		{Dispatch::UI::Side::BOTTOM, "bottom"},
		{Dispatch::UI::Side::START, "start"},
		{Dispatch::UI::Side::END, "end"},
		{Dispatch::UI::Side::LEFT, "left"},
		{Dispatch::UI::Side::RIGHT, "right"},
	});

	NLOHMANN_JSON_SERIALIZE_ENUM( Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType, {
		{ Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType::UNATTACHED, nullptr },
		{ Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType::FATHER, "father" },
		{ Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType::ELEMENT, "element" },
		{ Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType::SCREEN, "screen" },
	});

	NLOHMANN_JSON_SERIALIZE_ENUM( Dispatch::UI::Element::Status, {
		{ Dispatch::UI::Element::Status::REGULAR, "regular" },
		{ Dispatch::UI::Element::Status::HOVERED, "hovered" },
		{ Dispatch::UI::Element::Status::PRESSED, "pressed" },
		{ Dispatch::UI::Element::Status::SELECTED, "selected" },
		{ Dispatch::UI::Element::Status::DISABLED, "disabled" },
	});
};
