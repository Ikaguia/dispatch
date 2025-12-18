#pragma once

#include <memory>
#include <vector>
#include <fstream>

#include <raylib-cpp.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <Utils.hpp>
#include <Common.hpp>

namespace Dispatch::UI {
	enum struct FillType {
		NONE,
		FULL,
		GRADIENT,
		GRADIENT_H,
		GRADIENT_V,
	};

	class Element;

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
	public:
		int z_order = -1, roundnessSegments = 0;
		float borderThickness = 0.0f, roundness = 0.0f;
		std::string id, father_id, layout_name;
		std::vector<std::string> subElement_ids;
		raylib::Vector2 size, origSize, shadowOffset;
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
		virtual void handleInput();
		virtual void solveLayout();
		virtual void sortSubElements(bool z_order);

		virtual void to_json(nlohmann::json& j) const;
		virtual void from_json(const nlohmann::json& j);
	private:
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

		virtual void render() override;

		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
	};

	class TextBox : public virtual Box, public virtual Text {};

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

		virtual void render() override;
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

		virtual void render() override;
		virtual void from_json(const nlohmann::json& j) override;
		virtual void to_json(nlohmann::json& j) const override;
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

	extern std::map<std::string, std::map<std::string, std::unique_ptr<Element>>> elements;

	void loadLayout(std::string path);
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
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint& inst);
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint::ConstraintPart& inst);
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint::ConstraintPart& inst);
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
};
