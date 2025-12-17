#include <UI.hpp>

#include <algorithm>
#include <map>
#include <queue>
#include <set>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Dispatch::UI {
	std::map<std::string, std::map<std::string, std::unique_ptr<Element>>> elements;

	// Element
	const float Element::side(Side side, bool inner) const {
		const auto& rect = inner ? subElementsRect() : boundingRect();
		switch (side) {
			case Side::TOP:
				return rect.y;
			case Side::BOTTOM:
				return rect.y + rect.height;
			case Side::START:
				return rect.x;
			case Side::END:
				return rect.x + rect.width;
			default:
				throw std::invalid_argument("Invalid side");
		}
	}
	raylib::Vector2 Element::center() const { return anchor(); }
	raylib::Vector2 Element::anchor(Utils::Anchor anchor) const { return Utils::anchorPos(boundingRect(), anchor); };
	const raylib::Rectangle& Element::boundingRect() const {
		if (!initialized) throw std::runtime_error("Element bounding rect requested before initialization");
		return bounds;
	}
	const raylib::Rectangle& Element::subElementsRect() const {
		if (!initialized) throw std::runtime_error("Element inner bounding rect requested before initialization");
		return innerBounds;
	}

	const bool Element::colidesWith(const Element& other) const { return boundingRect().CheckCollision(other.boundingRect()); }
	const bool Element::colidesWith(const raylib::Vector2& other) const { return boundingRect().CheckCollision(other); }
	const bool Element::colidesWith(const raylib::Rectangle& other) const { return boundingRect().CheckCollision(other); }

	void Element::render() {
		raylib::Rectangle outterRect = boundingRect();
		raylib::Rectangle innerRect = subElementsRect();
		
		if ((shadowOffset.x != 0 || shadowOffset.y != 0) && shadowColor.a != 0) {
			raylib::Rectangle shadowRect{outterRect.GetPosition() + shadowOffset, outterRect.GetSize()};
			if (roundnessSegments > 0) shadowRect.DrawRounded(roundness, roundnessSegments, shadowColor);
			else shadowRect.Draw(shadowColor);
		}
		if (roundnessSegments > 0) {
			outterRect.DrawRounded(roundness, roundnessSegments, outterColor);
			innerRect.DrawRounded(roundness, roundnessSegments, innerColor);
		} else {
			outterRect.Draw(outterColor);
			innerRect.Draw(innerColor);
		}
		if (borderThickness > 0.0f) {
			if (roundnessSegments > 0) outterRect.DrawRoundedLines(roundness, roundnessSegments, borderThickness, borderColor);
			else outterRect.DrawLines(borderColor, borderThickness);
		}
		auto& lElements = elements.at(layout_name);
		for (const std::string& el_id : subElement_ids) lElements.at(el_id)->render();
	}
	void Element::handleInput() {
		auto& lElements = elements.at(layout_name);
		for (const std::string& el_id : subElement_ids) lElements.at(el_id)->handleInput();
	}
	void Element::solveLayout() {
		auto& lElements = elements.at(layout_name);
		if (origSize.x >= 0.0f && origSize.x <= 1.0f) {
			float maxW = father_id.empty() ? GetScreenWidth() : lElements.at(father_id)->subElementsRect().width;
			size.x = std::round(maxW * origSize.x - (outterMargin.x + outterMargin.z));
		}
		if (origSize.y >= 0.0f && origSize.y <= 1.0f) {
			float maxH = father_id.empty() ? GetScreenHeight() : lElements.at(father_id)->subElementsRect().height;
			size.y = std::round(maxH * origSize.y - (outterMargin.y + outterMargin.w));
		}

		bounds.width = size.x;
		bounds.height = size.y;
		for (int i = 0; i < 2; i++) {
			float& dest = i==0 ? bounds.x : bounds.y;
			const auto& constraint = i==0 ? horizontalConstraint : verticalConstraint;
			float startMargin = i==0 ? outterMargin.x : outterMargin.y;
			float endMargin = i==0 ? outterMargin.z : outterMargin.w;
			float sz = i==0 ? size.x : size.y;
			float screen = i==0 ? GetScreenWidth() : GetScreenHeight();

			float startPos, endPos;
			if (constraint.start.type == Constraint::ConstraintPart::UNATTACHED) startPos = nanf("");
			else if (constraint.start.type == Constraint::ConstraintPart::FATHER) startPos = lElements.at(father_id)->side(constraint.start.side, true);
			else if (constraint.start.type == Constraint::ConstraintPart::ELEMENT) startPos = lElements.at(constraint.start.element_id)->side(constraint.start.side, true);
			else if (constraint.start.type == Constraint::ConstraintPart::SCREEN) startPos = 0.0f;

			if (constraint.end.type == Constraint::ConstraintPart::UNATTACHED) endPos = nanf("");
			else if (constraint.end.type == Constraint::ConstraintPart::FATHER) endPos = lElements.at(father_id)->side(constraint.end.side, true);
			else if (constraint.end.type == Constraint::ConstraintPart::ELEMENT) endPos = lElements.at(constraint.end.element_id)->side(constraint.end.side, true);
			else if (constraint.end.type == Constraint::ConstraintPart::SCREEN) endPos = screen;

			bool startAttached = constraint.start.type != Constraint::ConstraintPart::UNATTACHED;
			bool endAttached = constraint.end.type != Constraint::ConstraintPart::UNATTACHED;

			if (startAttached && endAttached) {
				dest = startPos + startMargin + ((endPos - endMargin) - (startPos + startMargin) - sz) * constraint.ratio;
			} else if (startAttached) {
				dest = startPos + startMargin + constraint.offset;
			} else if (endAttached) {
				dest = endPos - endMargin - constraint.offset - sz;
			} else throw std::runtime_error("Element '" + id + "' missing constraint");
		}
		innerBounds = raylib::Rectangle{ bounds.x + innerMargin.x, bounds.y + innerMargin.y, bounds.width - innerMargin.x - innerMargin.z, bounds.height - innerMargin.y - innerMargin.w };

		initialized = true;
		sortSubElements(false);
		for (const std::string& el_id : subElement_ids) lElements.at(el_id)->solveLayout();
		sortSubElements(true);
	}

	void Element::sortSubElements(bool z_order) {
		auto& lElements = elements.at(layout_name);
		// Sort based on the z_order member
		if (z_order) {
			std::sort(subElement_ids.begin(), subElement_ids.end(), [&](const auto& lhs, const auto& rhs) { return lElements.at(lhs)->z_order < lElements.at(rhs)->z_order; });
			return;
		}

		// TOPOLOGICAL SORT (Constraint Order)
		const size_t N = subElement_ids.size();
		if (N == 0) return;

		// 1. Build Graph and Dependency Checks
		// Node Map: Maps ids to index (0 to N-1) for graph operations
		std::map<std::string, size_t> idToIndex;
		for (size_t i = 0; i < N; i++) idToIndex[subElement_ids[i]] = i;
		
		// Adjacency List: graph[i] contains indices of elements that depend on i.
		std::vector<std::vector<size_t>> adj(N);
		// In-Degree: Counts how many elements reference element i.
		std::vector<int> inDegree(N, 0);

		for (size_t i = 0; i < N; ++i) {
			// Element i is the dependent element (child)
			std::string dependent_id = subElement_ids[i];
			const Element& dependent = *lElements.at(dependent_id);

			// Helper function to check and build dependency for one constraint part
			auto processConstraint = [&](const Element::Constraint::ConstraintPart& part) {
				if (part.type == Element::Constraint::ConstraintPart::ELEMENT) {
					// Lock the weak_ptr to get the target element
					std::string target_id = part.element_id;
					// Check for non-sibling reference
					if (!idToIndex.contains(target_id)) throw std::runtime_error("Layout Error: Element '" + dependent_id + "' references non-sibling element.");

					size_t targetIndex = idToIndex[target_id];
					
					// Add edge: targetIndex -> i (target must be solved before i)
					adj[targetIndex].push_back(i);
					inDegree[i]++;
				}
				// FATHER attachments are handled by the Father's own solveLayout
				// so they don't count as sibling dependencies here.
			};

			// Process horizontal constraints
			processConstraint(dependent.horizontalConstraint.start);
			processConstraint(dependent.horizontalConstraint.end);

			// Process vertical constraints
			processConstraint(dependent.verticalConstraint.start);
			processConstraint(dependent.verticalConstraint.end);
		}

		// 2. Kahn's Topological Sort Algorithm
		std::queue<size_t> q;
		std::vector<std::string> sortedIds;
		sortedIds.reserve(N);

		// Initialize queue with all nodes having an in-degree of 0 (no dependencies)
		for (size_t i = 0; i < N; ++i) if (inDegree[i] == 0) q.push(i);

		while (!q.empty()) {
			size_t u_idx = q.front();
			q.pop();

			// Add the element to the sorted list
			sortedIds.push_back(subElement_ids[u_idx]);

			// Process neighbors (elements that depend on u)
			for (size_t v_idx : adj[u_idx]) {
				inDegree[v_idx]--;

				// If a neighbor's in-degree drops to 0, it is ready to be solved
				if (inDegree[v_idx] == 0) q.push(v_idx);
			}
		}

		// 3. Cycle Detection
		if (sortedIds.size() != N) throw std::runtime_error("Layout Error: Cyclic reference detected in sub-element constraints.");

		// 4. Update the vector
		subElement_ids = std::move(sortedIds);
	}

	void Text::render() {
		Element::render();
		raylib::Rectangle rect = boundingRect();
		Utils::drawTextAnchored(
			text,
			rect,
			textAnchor,
			font,
			textColor,
			fontSize,
			spacing,
			{},
			rect.width
		);
	}

	std::unique_ptr<Element> elementFactory(std::string type) {
		if (type == "ELEMENT") return std::make_unique<Element>();
		else if (type == "BOX") return std::make_unique<Box>();
		else if (type == "TEXT") return std::make_unique<Text>();
		else if (type == "TEXTBOX") return std::make_unique<TextBox>();
		else throw std::invalid_argument("Invalid type in JSON layout");
	}

	void loadLayout(std::string path) {
		json data_array = Utils::readJsonFile(path);
		size_t name_start = path.find_last_of("/");
		size_t type_start = path.find_last_of(".");
		std::string layout_name = name_start == std::string::npos ? path : path.substr(name_start+1, type_start-name_start-1);
		Utils::println("Reading layout '{}'", layout_name);

		if (!data_array.is_array()) throw std::runtime_error("Layout Error: Top-level JSON must be an array of elements.");
		if (elements.count(layout_name)) throw std::runtime_error("Layout Error: Duplicated layout name");

		auto& lElements = elements[layout_name];
		std::set<std::string> top_level_ids;

		for (const auto& data : data_array) {
			std::string type = data.at("type").get<std::string>();
			auto el = elementFactory(type);
			try { data.get_to(*el); }
			catch (const std::exception& e) { throw std::runtime_error("Layout Error: Failed to deserialize element of type '" + type + "'. " + e.what()); }
			el->layout_name = layout_name;
			if (lElements.count(el->id)) throw std::runtime_error("Layout Error: Element with ID '" + el->id + "' already exists in the map.");
			if (el->father_id.empty()) top_level_ids.insert(el->id);

			if (lElements.count(el->father_id)) {
				const auto& father = lElements[el->father_id];
				if (!std::any_of(father->subElement_ids.begin(), father->subElement_ids.end(), [&](std::string id){ return id == el->id; })) {
					throw std::runtime_error(std::format("Layout Error: Father/Subelements mismatch between '{}' and '{}'", el->id, el->father_id));
				}
			}
			for (const auto& subElement_id : el->subElement_ids) {
				if (lElements.count(subElement_id) && lElements[subElement_id]->father_id != el->id) {
					throw std::runtime_error(std::format("Layout Error: Father/Subelements mismatch between '{}' and '{}'", el->id, subElement_id));
				}
			}

			lElements[el->id] = std::move(el);
		}

		if (!data_array.empty() && top_level_ids.empty()) throw std::runtime_error("Layout Error: No root element found");
		for (const std::string& id : top_level_ids) lElements.at(id)->solveLayout();
	}


	// JSON Serialization Macros
	#define READ(j, var)		inst.var = j.value(#var, inst.var)
	#define READREQ(j, var) { \
								if (j.contains(#var)) inst.var = j.at(#var).get<decltype(inst.var)>(); \
								else throw std::invalid_argument("Hero '" + std::string(#var) + "' cannot be empty"); \
							}
	#define READ2(j, var, key)	inst.var = j.value(#key, inst.var)
	#define READREQ2(j, var, key) { \
								if (j.contains(#key)) inst.var = j.at(#key).get<decltype(inst.var)>(); \
								else throw std::invalid_argument("Hero '" + std::string(#key) + "' cannot be empty"); \
							}
	#define WRITE(var)			j[#var] = inst.var;
	#define WRITE2(var, key)	j[#key] = inst.var;

	// JSON Serialization
	void Element::to_json(json& j) const {
		auto& inst = *this;
		WRITE(z_order);
		WRITE(roundnessSegments);
		WRITE(borderThickness);
		WRITE(roundness);
		WRITE(id);
		WRITE(father_id);
		// WRITE(layout_name);
		WRITE(subElement_ids);
		WRITE(size);
		WRITE(shadowOffset);
		WRITE(outterMargin);
		WRITE(innerMargin);
		WRITE(innerColor);
		WRITE(outterColor);
		WRITE(borderColor);
		WRITE(shadowColor);
		WRITE(verticalConstraint);
		WRITE(horizontalConstraint);
	}
	void Element::from_json(const json& j) {
		auto& inst = *this;
		READ(j, z_order);
		READ(j, roundnessSegments);
		READ(j, borderThickness);
		READ(j, roundness);
		READREQ(j, id);
		READ(j, father_id);
		// READ(j, layout_name);
		READ(j, subElement_ids);
		READREQ(j, size);
		READ(j, shadowOffset);
		READ(j, outterMargin);
		READ(j, innerMargin);
		READ(j, innerColor);
		READ(j, outterColor);
		READ(j, borderColor);
		READ(j, shadowColor);
		READREQ(j, verticalConstraint);
		READREQ(j, horizontalConstraint);
		inst.origSize = inst.size;
	}
	void Text::to_json(json& j) const {
		auto& inst = *this;
		Element::to_json(j);
		WRITE(fontSize);
		WRITE(spacing);
		WRITE(text);
		WRITE(fontName);
		WRITE(fontColor);
		WRITE(textAnchor);
	}
	void Text::from_json(const json& j) {
		auto& inst = *this;
		Element::from_json(j);
		READ(j, fontSize);
		READ(j, spacing);
		READREQ(j, text);
		READ(j, fontName);
		READ(j, fontColor);
		READ(j, textAnchor);
	}
}

namespace nlohmann {
	inline void to_json(json& j, const Dispatch::UI::Element::Constraint& inst) {
		WRITE(offset);
		WRITE(ratio);
		WRITE(start);
		WRITE(end);
	};
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint& inst) {
		READ(j, offset);
		READ(j, ratio);
		READ(j, start);
		READ(j, end);
	}
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Element::Constraint::ConstraintPart& inst) {
		WRITE(type);
		WRITE(element_id);
		WRITE(side);
	}
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Element::Constraint::ConstraintPart& inst) {
		READREQ(j, type);
		READ(j, element_id);
		READREQ(j, side);
	}
};
