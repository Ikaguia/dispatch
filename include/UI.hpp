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
	private:
		raylib::Rectangle bounds, innerBounds;
		bool initialized = false;
	};

	class Box : public Element {
	public:
		Box() {
			borderThickness = 1.0f;
			innerColor = bgLgt;
			outterColor = bgMed;
			borderColor = BLACK;
		}
	};

	extern std::map<std::string, std::map<std::string, std::unique_ptr<Element>>> elements;

	void loadLayout(std::string path);

}

namespace nlohmann {
	// JSON Serialization
	void to_json(nlohmann::json& j, const Dispatch::UI::Element& element);
	void from_json(const nlohmann::json& j, Dispatch::UI::Element& element);
	void to_json(nlohmann::json& j, const Dispatch::UI::Box& box);
	void from_json(const nlohmann::json& j, Dispatch::UI::Box& box);
	void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint& constraint);
	void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint& constraint);
	void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint::ConstraintPart& constraintPart);
	void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint::ConstraintPart& constraintPart);

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

	// class Box : Element {
	// public:
	// 	raylib::Vector2 size;
	// 	raylib::Vector4 outMargin, inMargin;
	// 	struct Edge {
	// 		float round, thickness;
	// 		raylib::Color color;
	// 	} edge;
	// 	struct Fill {
	// 		FillType fillType;
	// 		std::vector<raylib::Color> color;
	// 	} fillMargin, fillInner;

	// 	const raylib::Vector2& position() const override;
	// 	const raylib::Vector2& anchor(raylib::Vector2 offset, Utils::Anchor anchor = Utils::Anchor::center) override;
	// 	const raylib::Rectangle& anchor(raylib::Rectangle rect, Utils::Anchor anchor = Utils::Anchor::center) override;
	// 	const raylib::Rectangle& boundingRect() const override;

	// 	const bool colidesWith(const Element& other) const override;
	// 	const bool colidesWith(const raylib::Vector2& other) const override;
	// 	const bool colidesWith(const raylib::Rectangle& other) const override;

	// 	void render(const raylib::Vector2 offset = raylib::Vector2{0.0f, 0.0f}) override;
	// 	void handleInput() override;
	// };
};
