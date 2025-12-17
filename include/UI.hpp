#pragma once

#include <memory>
#include <vector>
#include <fstream>

#include <raylib-cpp.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>
#include <Utils.hpp>

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
		int z_order = -1;
		std::string id, father_id, layout_name;
		std::vector<std::string> subElement_ids;
		raylib::Vector2 size, origSize, shadowOffset;
		raylib::Vector4 outterMargin, innerMargin;
		raylib::Color shadowColor = ColorAlpha(BLACK, 0.3f);
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

	extern std::map<std::string, std::map<std::string, std::unique_ptr<Element>>> elements;

	void loadLayout(std::string path);

	// JSON Serialization
	void to_json(nlohmann::json& j, const Element& element);
	void from_json(const nlohmann::json& j, Element& element);
	void to_json(nlohmann::json& j, const Element::Constraint& constraint);
	void from_json(const nlohmann::json& j, Element::Constraint& constraint);
	void to_json(nlohmann::json& j, const Element::Constraint::ConstraintPart& constraintPart);
	void from_json(const nlohmann::json& j, Element::Constraint::ConstraintPart& constraintPart);

	NLOHMANN_JSON_SERIALIZE_ENUM( Side, {
		{Side::INVALID, nullptr},
		{Side::TOP, "top"},
		{Side::BOTTOM, "bottom"},
		{Side::START, "start"},
		{Side::END, "end"},
		{Side::LEFT, "left"},
		{Side::RIGHT, "right"},
	});

	NLOHMANN_JSON_SERIALIZE_ENUM( Element::Constraint::ConstraintPart::ConstraintType, {
		{ Element::Constraint::ConstraintPart::ConstraintType::UNATTACHED, nullptr },
		{ Element::Constraint::ConstraintPart::ConstraintType::FATHER, "father" },
		{ Element::Constraint::ConstraintPart::ConstraintType::ELEMENT, "element" },
		{ Element::Constraint::ConstraintPart::ConstraintType::SCREEN, "screen" },
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
