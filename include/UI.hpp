#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <raylib-cpp.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <Utils.hpp>
#include <Common.hpp>

namespace Dispatch::UI {
	class Element; class Style;

	class Layout {
	public:
		std::unordered_map<std::string, std::unique_ptr<Element>> elements;
		std::unordered_map<std::string, std::unique_ptr<Style>> styles;
		std::unordered_set<std::string> rootElements, hovered, clicked, pressed, unhover, release, dataChanged, needsRebuild;
		std::unordered_map<std::string, nlohmann::json> sharedData;
		std::unordered_map<std::string, std::unordered_set<std::string>> sharedDataListeners;
		std::string path;

		Layout(const std::string& path);
		std::unordered_set<std::string> load(nlohmann::json& data);
		std::unordered_set<std::string> reload(const std::string& path);
		std::unordered_set<std::string> reload(nlohmann::json& data);

		void sync();
		void render();
		void handleInput();
		void resetInput();
		void updateSharedData(const std::string& key, const nlohmann::json& value);
		void deleteSharedData(const std::string& key);
		void registerSharedDataListener(const std::string& key, const std::string& element_id);
		void unregisterSharedDataListener(const std::string& key, const std::string& element_id);
		void removeElement(const std::string& id, const std::string& loop="");
	};

	enum struct Side {
		TOP,
		BOTTOM,
		START,
		END,
		LEFT = START,
		RIGHT = END,
		CENTER_H,
		CENTER_V,
		INVALID = -1
	};

	enum struct Orientation {
		VERTICAL,
		HORIZONTAL
	};

	enum struct Bound {
		REGULAR,
		OUTTER,
		INNER
	};

	class Style {
	public:
		std::string id, father_id;
		nlohmann::json orig;

		void to_json(nlohmann::json& j) const;
		void from_json(const nlohmann::json& j);
	};

	class Element {
	protected:
		nlohmann::json orig;
		std::unordered_set<std::string> dynamic_vars;
	public:
		Layout* layout=nullptr;
		bool visible=true, hovered=false, clicked=false, pressed=false, unhover=false, release=false, resortSub=false;
		int z_order=-1, roundnessSegments=0;
		float borderThickness=0.0f, roundness=0.0f;
		std::string id, father_id;
		std::vector<std::string> subElement_ids, style_ids;
		raylib::Vector2 size, shadowOffset;
		raylib::Vector4 outterMargin, innerMargin;
		std::vector<raylib::Color> innerColors, outterColors;
		raylib::Color borderColor, shadowColor=shadow;
		struct Constraint {
			float offset=0.0f, ratio=0.5f;
			struct ConstraintPart {
				enum ConstraintType { UNATTACHED, FATHER, ELEMENT, SCREEN } type=UNATTACHED;
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

		virtual float side(Side side, Bound bound = Bound::REGULAR) const;
		virtual raylib::Vector2 center() const;
		virtual raylib::Vector2 anchor(Utils::Anchor anchor = Utils::Anchor::center) const;
		virtual const raylib::Rectangle& rect(Bound bound = Bound::REGULAR) const;
		virtual const raylib::Rectangle& outterRect() const { return rect(Bound::OUTTER); }
		virtual const raylib::Rectangle& innerRect() const { return rect(Bound::INNER); }

		virtual bool colidesWith(const Element& other) const;
		virtual bool colidesWith(const raylib::Vector2& other) const;
		virtual bool colidesWith(const raylib::Rectangle& other) const;

		virtual void preInit();
		virtual void init();
		virtual void render();
		virtual void _render();
		virtual void handleInput(raylib::Vector2 offset={});
		virtual void _handleInput(raylib::Vector2 offset={});
		virtual void resetInput();
		virtual void solveLayout();
		virtual void solveSize();
		virtual void sortSubElements(bool z_order);
		virtual void onSharedDataUpdate(const std::string& /* key */, const nlohmann::json& /* value */) {}
		virtual std::string sharedDataDefault() const;
		virtual void changeStatus(Status st, bool force=false);
		virtual void rebuild() {};
		virtual void applyStyles(bool force=false);
		virtual void applyStyle(const std::string& st_id, bool force=false);
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value);

		virtual void to_json(nlohmann::json& j) const;
		virtual void from_json(const nlohmann::json& j);

		void parseString(const std::string& orig);
		std::string updateString(const std::string& orig);
	protected:
		std::unordered_map<Bound, raylib::Rectangle> boundsMap;
		bool initialized = false;
	};

	class Box : public virtual Element {
	public:
		Box() {
			borderThickness = 1.0f;
			innerColors = {bgLgt, bgLgt, bgMed, bgLgt};
			outterColors = {bgMed};
			borderColor = BLACK;
		}
	};

	class Text : public virtual Element {
	public:
		raylib::Font font = GetFontDefault();
		int fontSize = 16;
		float spacing = 1.0f;
		std::string text, fontName;
		raylib::Color fontColor = textColor;
		Utils::Anchor textAnchor = Utils::Anchor::center;

		virtual void preInit() override;
		virtual void _render() override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value) override;

		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
	};

	class TextBox : public virtual Box, public virtual Text {};

	class Button : public virtual TextBox {
	public:
		struct StatusChanges {
			std::vector<raylib::Color> inner, outter;
			raylib::Color border, text;
			float size_mult;
			int z_order_offset;
			StatusChanges(std::vector<raylib::Color> i, std::vector<raylib::Color> o, raylib::Color b, raylib::Color t, float s, int z) : inner{i}, outter{o}, border{b}, text{t}, size_mult{s}, z_order_offset{z} {}
			StatusChanges() = default;
		};
		std::unordered_map<Status, StatusChanges> statusChanges;
		float size_mult = 1.0f;

		Button() : TextBox() {
			statusChanges = std::unordered_map<Status, StatusChanges>{
				{Status::REGULAR, {{bgLgt, bgLgt, bgMed, bgLgt}, {bgMed}, {BLACK}, textColor, 1.0f, 0}},
				{Status::HOVERED, {{SKYBLUE}, {BLUE}, {BLACK}, textColor, 1.1f, 30}},
				{Status::PRESSED, {{BLUE}, {DARKBLUE}, {BLACK}, textColor, 1.1f, 20}},
				{Status::SELECTED, {{ORANGE}, {BROWN}, {BLACK}, WHITE, 1.0f, 10}},
				{Status::DISABLED, {{bgDrk}, {DARKGRAY}, {BLACK}, LIGHTGRAY, 1.0f, -10}}
			};
		}

		virtual void solveSize() override;
		virtual void changeStatus(Status st, bool force=false) override;
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value) override;

		virtual void to_json(nlohmann::json& j) const override;
		virtual void from_json(const nlohmann::json& j) override;
	};

	class RadioButton : public virtual Button {
	public:
		std::string key;

		virtual void preInit() override;
		virtual void _handleInput(raylib::Vector2 offset={}) override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;

		virtual void to_json(nlohmann::json& j) const override;
		virtual void from_json(const nlohmann::json& j) override;
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
		std::string imgPath, imgKey, placeholderPath, placeholderKey;
		Utils::FillType fillType = Utils::FillType::fill;
		Utils::Anchor imageAnchor = Utils::Anchor::center;
		raylib::Color tintColor = WHITE;

		virtual void init() override;
		virtual void _render() override;
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value) override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
		virtual std::string sharedDataDefault() const override;
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
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value) override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
	};

	class AttrGraph : public virtual RadarGraph {
	public:
		AttrGraph() : Element(), RadarGraph() {
			for (const Attribute attr : Attribute::Values) segments.push_back({std::string(attr.toString()), std::string(attr.toIcon())});
			innerColors = {BLACK};
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
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value) override;

		virtual void to_json(nlohmann::json& j) const override;
		virtual void from_json(const nlohmann::json& j) override;
	};

	class DataInspector : public virtual ScrollBox {
	public:
		std::string dataPath;
		std::unordered_set<std::string> fixedChilds;
		int labelFontSize = 14;
		int valueFontSize = 16;
		float itemSpacing = 10.0f;
		Orientation orientation=Orientation::VERTICAL;

		virtual void init() override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
		virtual bool applyStylePart(const std::string& key, const nlohmann::json& value) override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void rebuild() override;
	};

	class DataArray : public virtual ScrollBox {
		std::vector<std::string> curChildren;
	public:
		std::string dataPath;
		nlohmann::json childTemplate;

		virtual void init() override;
		virtual void onSharedDataUpdate(const std::string& key, const nlohmann::json& value) override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
		virtual void rebuild() override;
	};
}

namespace nlohmann {
	// JSON Serialization
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Style& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Style& inst) { inst.from_json(j); }
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
	inline void to_json(nlohmann::json& j, const Dispatch::UI::DataInspector& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::DataInspector& inst) { inst.from_json(j); }
	inline void to_json(nlohmann::json& j, const Dispatch::UI::DataArray& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Dispatch::UI::DataArray& inst) { inst.from_json(j); }
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
		{Dispatch::UI::Side::INVALID, "invalid"},
		{Dispatch::UI::Side::TOP, "top"},
		{Dispatch::UI::Side::BOTTOM, "bottom"},
		{Dispatch::UI::Side::START, "start"},
		{Dispatch::UI::Side::END, "end"},
		{Dispatch::UI::Side::LEFT, "left"},
		{Dispatch::UI::Side::RIGHT, "right"},
		{Dispatch::UI::Side::CENTER_H, "center_h"},
		{Dispatch::UI::Side::CENTER_V, "center_v"},
	});

	NLOHMANN_JSON_SERIALIZE_ENUM( Dispatch::UI::Orientation, {
		{Dispatch::UI::Orientation::VERTICAL, "vertical"},
		{Dispatch::UI::Orientation::HORIZONTAL, "horizontal"},
	});

	NLOHMANN_JSON_SERIALIZE_ENUM( Dispatch::UI::Bound, {
		{Dispatch::UI::Bound::REGULAR, "regular"},
		{Dispatch::UI::Bound::OUTTER, "outter"},
		{Dispatch::UI::Bound::INNER, "inner"}
	});

	NLOHMANN_JSON_SERIALIZE_ENUM( Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType, {
		{ Dispatch::UI::Element::Constraint::ConstraintPart::ConstraintType::UNATTACHED, "unattached" },
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
