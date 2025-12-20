#include <UI.hpp>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <regex>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Dispatch::UI {
	std::unique_ptr<Element> elementFactory(std::string type) {
		if (type == "ELEMENT") return std::make_unique<Element>();
		else if (type == "BOX") return std::make_unique<Box>();
		else if (type == "TEXT") return std::make_unique<Text>();
		else if (type == "TEXTBOX") return std::make_unique<TextBox>();
		else if (type == "BUTTON") return std::make_unique<Button>();
		else if (type == "RADIOBUTTON") return std::make_unique<RadioButton>();
		else if (type == "CIRCLE") return std::make_unique<Circle>();
		else if (type == "TEXTCIRCLE") return std::make_unique<TextCircle>();
		else if (type == "IMAGE") return std::make_unique<Image>();
		else if (type == "RADARGRAPH") return std::make_unique<RadarGraph>();
		else if (type == "ATTRGRAPH") return std::make_unique<AttrGraph>();
		else throw std::invalid_argument("Invalid type in JSON layout");
	}

	// Layout
	Layout::Layout(std::string path) {
		json data_array = Utils::readJsonFile(path);
		Utils::println("Reading layout '{}'", path);

		if (!data_array.is_array()) throw std::runtime_error("Layout Error: Top-level JSON must be an array of elements.");

		for (const auto& data : data_array) {
			std::string type = data.at("type").get<std::string>();
			auto el = elementFactory(type);
			el->layout = this;
			try { data.get_to(*el); }
			catch (const std::exception& e) { throw std::runtime_error("Layout Error: Failed to deserialize element of type '" + type + "'. " + e.what()); }
			if (elements.count(el->id)) throw std::runtime_error("Layout Error: Element with ID '" + el->id + "' already exists in the map.");
			if (el->father_id.empty()) rootElements.insert(el->id);

			if (elements.count(el->father_id)) {
				const auto& father = elements[el->father_id];
				if (!std::any_of(father->subElement_ids.begin(), father->subElement_ids.end(), [&](std::string id){ return id == el->id; })) {
					throw std::runtime_error(std::format("Layout Error: Father/Subelements mismatch between '{}' and '{}'", el->id, el->father_id));
				}
			}
			for (const auto& subElement_id : el->subElement_ids) {
				if (elements.count(subElement_id) && elements[subElement_id]->father_id != el->id) {
					throw std::runtime_error(std::format("Layout Error: Father/Subelements mismatch between '{}' and '{}'", el->id, subElement_id));
				}
			}

			elements[el->id] = std::move(el);
		}

		if (!data_array.empty() && rootElements.empty()) throw std::runtime_error("Layout Error: No root element found");
		for (const std::string& id : rootElements) elements.at(id)->solveLayout();
	}
	void Layout::render() { for (const std::string& id : rootElements) elements.at(id)->render(); }
	void Layout::handleInput() {
		hovered.clear();
		clicked.clear();
		pressed.clear();
		unhover.clear();
		release.clear();
		dataChanged.clear();
		for (const std::string& id : rootElements) elements.at(id)->handleInput();
	}
	void Layout::updateSharedData(const std::string& key, const json& value) {
		if (value == sharedData[key]) return;
		sharedData[key] = value;
		dataChanged.insert(key);
		for (const std::string& id : sharedDataListeners[key]) elements.at(id)->onSharedDataUpdate(key, value);
	}
	void Layout::deleteSharedData(const std::string& key) {
		if (sharedData.find(key) == sharedData.end()) return;
		sharedData.erase(key);
		for (const std::string& id : sharedDataListeners[key]) elements.at(id)->onSharedDataUpdate(key, nullptr);
	}
	void Layout::registerSharedDataListener(const std::string& key, const std::string& element_id) {
		Utils::println("Registering element '{}' as listener for shared data key '{}'", element_id, key);
		sharedDataListeners[key].insert(element_id);
	}

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
		if (!visible) return;
		_render();
		auto& elements = layout->elements;
		for (const std::string& el_id : subElement_ids) elements.at(el_id)->render();
	}
	void Element::_render() {
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
	}
	void Element::handleInput() {
		auto& elements = layout->elements;
		for (const std::string& el_id : subElement_ids) elements.at(el_id)->handleInput();

		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		bool wasHovered = hovered, wasPressed = pressed;
		hovered = colidesWith(mousePos);
		clicked = hovered && !wasPressed && raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT);
		pressed = clicked || (wasPressed && raylib::Mouse::IsButtonDown(MOUSE_BUTTON_LEFT));
		unhover = wasHovered && !hovered;
		release = wasPressed && !pressed && hovered;
		if (hovered) layout->hovered.insert(id);
		if (clicked) layout->clicked.insert(id);
		if (pressed) layout->pressed.insert(id);
		if (unhover) layout->unhover.insert(id);
		if (release) layout->release.insert(id);

		if (status != Status::DISABLED && status != Status::SELECTED) {
			if (clicked || pressed) changeStatus(Status::PRESSED);
			else if (hovered) changeStatus(Status::HOVERED);
			else changeStatus(Status::REGULAR);
		}
	}
	void Element::solveLayout() {
		auto& elements = layout->elements;
		solveSize();
		for (int i = 0; i < 2; i++) {
			float& dest = i==0 ? bounds.x : bounds.y;
			const auto& constraint = i==0 ? horizontalConstraint : verticalConstraint;
			float startMargin = i==0 ? outterMargin.x : outterMargin.y;
			float endMargin = i==0 ? outterMargin.z : outterMargin.w;
			float sz = i==0 ? size.x : size.y;
			float screen = i==0 ? GetScreenWidth() : GetScreenHeight();

			float startPos, endPos;
			if (constraint.start.type == Constraint::ConstraintPart::UNATTACHED) startPos = nanf("");
			else if (constraint.start.type == Constraint::ConstraintPart::FATHER) startPos = elements.at(father_id)->side(constraint.start.side, true);
			else if (constraint.start.type == Constraint::ConstraintPart::ELEMENT) startPos = elements.at(constraint.start.element_id)->side(constraint.start.side, true);
			else if (constraint.start.type == Constraint::ConstraintPart::SCREEN) startPos = 0.0f;

			if (constraint.end.type == Constraint::ConstraintPart::UNATTACHED) endPos = nanf("");
			else if (constraint.end.type == Constraint::ConstraintPart::FATHER) endPos = elements.at(father_id)->side(constraint.end.side, true);
			else if (constraint.end.type == Constraint::ConstraintPart::ELEMENT) endPos = elements.at(constraint.end.element_id)->side(constraint.end.side, true);
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
		for (const std::string& el_id : subElement_ids) elements.at(el_id)->solveLayout();
		sortSubElements(true);
	}
	void Element::solveSize() {
		auto& elements = layout->elements;
		raylib::Vector2 origSize = orig.at("size").get<raylib::Vector2>();
		if (origSize.x >= 0.0f && origSize.x <= 1.0f) {
			float maxW = father_id.empty() ? GetScreenWidth() : elements.at(father_id)->subElementsRect().width;
			size.x = std::round(maxW * origSize.x - (outterMargin.x + outterMargin.z));
		}
		if (origSize.y >= 0.0f && origSize.y <= 1.0f) {
			float maxH = father_id.empty() ? GetScreenHeight() : elements.at(father_id)->subElementsRect().height;
			size.y = std::round(maxH * origSize.y - (outterMargin.y + outterMargin.w));
		}
		bounds.width = size.x;
		bounds.height = size.y;
	}
	void Element::sortSubElements(bool z_order) {
		auto& elements = layout->elements;
		// Sort based on the z_order member
		if (z_order) {
			std::sort(subElement_ids.begin(), subElement_ids.end(), [&](const auto& lhs, const auto& rhs) { return elements.at(lhs)->z_order < elements.at(rhs)->z_order; });
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
			const Element& dependent = *elements.at(dependent_id);

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
	void Element::changeStatus(Status st, bool force) { status = st; }
	// Text
	void Text::_render() {
		Element::_render();
		raylib::Rectangle rect = boundingRect();
		Utils::drawTextAnchored(
			text,
			rect,
			textAnchor,
			font,
			fontColor,
			fontSize,
			spacing,
			{},
			rect.width
		);
	}
	void Text::onSharedDataUpdate(const std::string& key, const json& value) { updateText(); }
	void Text::parseText() {
		static const std::regex pattern{R"(\{@(\w+)\})"};
		std::string origText = orig.at("text").get<std::string>();
		std::sregex_token_iterator iter(origText.begin(), origText.end(), pattern, 1);
		std::sregex_token_iterator end;

		if (iter == end) return;
		dynamic_vars.insert("text");

		for (; iter != end; ++iter) layout->registerSharedDataListener(*iter, id);
		updateText();
	}
	void Text::updateText() {
		if (!dynamic_vars.count("text")) return;

		static const std::regex pattern{R"(\{@(\w+)\})"};
		std::string origText = orig.at("text").get<std::string>();

		std::string result;
		result.reserve(origText.size());

		auto it = std::sregex_iterator(origText.begin(), origText.end(), pattern);
		auto end = std::sregex_iterator();
		
		size_t lastPos = 0;

		for (; it != end; ++it) {
			const std::smatch& match = *it;
			
			// 1. Append the text between the last match and this one
			result.append(origText, lastPos, match.position(0) - lastPos);
			
			// 2. Lookup the replacement
			std::string key = match[1].str();
			auto dataIt = layout->sharedData.find(key);
			
			if (dataIt != layout->sharedData.end()) result.append(dataIt->second.get<std::string>());
			else result.append("???");

			// 3. Move the cursor forward
			lastPos = match.position(0) + match.length(0);
		}

		// 4. Append any remaining text after the last match
		result.append(origText, lastPos, std::string::npos);
		
		text = std::move(result);
	}
	// Image
	void Image::_render() {
		Utils::drawTextureAnchored(
			texture,
			boundingRect(),
			tintColor,
			fillType,
			imageAnchor
		);
	}
	// Button
	void Button::solveSize() {
		Element::solveSize();
		size.x *= size_mult;
		size.y *= size_mult;
		bounds.width *= size_mult;
		bounds.height *= size_mult;
	}
	void Button::changeStatus(Status st, bool force) {
		Status oldStatus = status;
		Element::changeStatus(st);
		if (force || (status != oldStatus)) {
			StatusChanges& sc = statusChanges[status];
			innerColor = sc.inner;
			outterColor = sc.outter;
			borderColor = sc.border;
			fontColor = sc.text;
			size_mult = sc.size_mult;
			if (initialized) solveLayout();
		}
	}
	void RadioButton::handleInput() {
		Button::handleInput();
		if (release) layout->updateSharedData(key, id);
	}
	void RadioButton::onSharedDataUpdate(const std::string& k, const nlohmann::json& value) {
		Button::onSharedDataUpdate(k, value);
		if (key != k) return;
		if (value == id) changeStatus(Status::SELECTED);
		else if (status == Status::SELECTED) changeStatus(Status::REGULAR);
	}
	// RadarGraph
	void RadarGraph::_render() {
		raylib::Rectangle rect = subElementsRect();
		raylib::Vector2 center = this->center();
		float sideLength = std::min(rect.width, rect.height) / 2.0f;
		if (drawIcons) sideLength *= 0.8f;

		const int sides = segments.size();
		float baseRotation = 90.0f + 180.0f / sides;
		DrawPoly(center, sides, sideLength+1, baseRotation, innerColor);
		DrawPolyLines(center, sides, sideLength-1, baseRotation, borderColor);
		for (int i = 1; i < 5; i++) DrawPolyLines(center, sides, i * sideLength / 5, baseRotation, bgLines);
		for (int i = 0; i < sides; i++) center.DrawLine(center + raylib::Vector2{0, -sideLength}.Rotate(i * 2.0f * PI / sides), bgLines);
		if (drawIcons) for (int i = 0; i < sides; i++) {
			std::string icon = segments[i].icon;
			auto pos = center + raylib::Vector2{0, -(sideLength * 1.33f)}.Rotate(i * 2.0f * PI / sides);
			pos.DrawCircle(18, innerColor);
			DrawCircleLines(pos.x, pos.y, 16, borderColor);
			Utils::drawTextCenteredShadow(icon, pos, emojiFont, 26);
		}
		for (auto [values, color] : groups) {
			if (values.size() != (size_t)sides) continue;
			std::vector<raylib::Vector2> points;
			for (int i = 0; i < sides; i++) {
				float value = values[i];
				value = std::clamp(value, 0.0f, maxValue);
				auto cur = center + raylib::Vector2{0, -(sideLength * value / maxValue)}.Rotate(i * 2.0f * PI / sides);
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
				float value = values[idx];
				auto point = points[idx];
				value = std::clamp(value, 0.0f, maxValue);
				if (drawLabels) {
					point.DrawCircle(6, color);
					auto text = std::format("{:.{}f}", value, precision);
					auto sz = Dispatch::UI::defaultFont.MeasureText(text, 12, 2);
					Dispatch::UI::defaultFont.DrawText(text, point - sz/2, 12, 2, BLACK);
				} else if (sideLength > 20) point.DrawCircle(3, color.Fade(0.4f));
			}
		}
	}
	void RadarGraph::onSharedDataUpdate(const std::string& key, const nlohmann::json& value) {
		int idx = 0;
		if (dynamic_vars.count("groups")) {
			for (auto& [values, color] : groups) {
				if (dynamic_vars.count(std::format("groups[{}]", idx)) && orig["groups"].at(idx)["values"].get<std::string>() == std::format("{{@{}}}", key)) {
					json jvalues = value.contains("values") ? value["values"] : value;
					json jcolor = value.contains("color") ? value["color"] : json();
					if (!jvalues.is_null()) {
						values.clear();
						if (jvalues.is_array()) for (const auto& jval : jvalues) values.push_back(jval.get<float>());
						else if (jvalues.is_object()) {
							for (const auto& seg : segments) {
								auto lclabel = Utils::toLower(seg.label);
								if (jvalues.contains(lclabel)) values.push_back(jvalues.at(lclabel).get<float>());
								else values.push_back(0.0f);
							}
						} else throw std::runtime_error("RadarGraph '" + id + "' group values must be an array or object");
					}
					if (!jcolor.is_null()) color = jcolor.get<raylib::Color>();
				}
				idx++;
			}
		}
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
		WRITE(visible);
		WRITE(z_order);
		WRITE(roundnessSegments);
		WRITE(borderThickness);
		WRITE(roundness);
		WRITE(id);
		WRITE(father_id);
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
		orig = j;
		auto& inst = *this;
		READ(j, visible);
		READ(j, z_order);
		READ(j, roundnessSegments);
		READ(j, borderThickness);
		READ(j, roundness);
		READREQ(j, id);
		READ(j, father_id);
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
		// font = Utils::getFont(fontName);
		parseText();
	}
	void Button::to_json(nlohmann::json& j) const {
		auto& inst = *this;
		Text::to_json(j);
		WRITE(statusChanges);
	}
	void Button::from_json(const nlohmann::json& j) {
		Text::from_json(j);
		if (j.contains("statusChanges")) {
			for (Element::Status st : Element::statuses) {
				std::string sts{json{st}.dump()}; sts = sts.substr(2, sts.size()-4);
				if (j["statusChanges"].contains(sts)) j["statusChanges"][sts].get_to(statusChanges[st]);
			}
			changeStatus(Status::REGULAR, true);
		}
	}
	void RadioButton::to_json(nlohmann::json& j) const {
		auto& inst = *this;
		Button::to_json(j);
		WRITE(key);
	}
	void RadioButton::from_json(const nlohmann::json& j) {
		auto& inst = *this;
		Button::from_json(j);
		READ(j, key);
		if (j.contains("selected")) {
			layout->updateSharedData(key, id);
			changeStatus(Status::SELECTED);
		}
		layout->registerSharedDataListener(key, id);
	}
	void Circle::to_json(json& j) const {
		auto& inst = *this;
		Element::to_json(j);
		if (size.x == size.y) {
			WRITE2(size.x, radius);
			j.erase("size");
		}
	}
	void Circle::from_json(const json& j) {
		json j_copy = j;
		if (j_copy.contains("radius")) {
			float radius = j_copy["radius"].get<float>();
			j_copy["size"] = raylib::Vector2{radius, radius};
		}
		Element::from_json(j_copy);
	}
	void TextCircle::to_json(json& j) const {
		auto& inst = *this;
		Text::to_json(j);
		if (size.x == size.y) {
			WRITE2(size.x, radius);
			j.erase("size");
		}
	}
	void TextCircle::from_json(const json& j) {
		json j_copy = j;
		if (j_copy.contains("radius")) {
			float radius = j_copy["radius"].get<float>();
			j_copy["size"] = raylib::Vector2{radius, radius};
		}
		Text::from_json(j_copy);
	}
	void Image::to_json(json& j) const {
		auto& inst = *this;
		Element::to_json(j);
		WRITE(path);
		WRITE(fillType);
		WRITE(imageAnchor);
		WRITE(tintColor);
	}
	void Image::from_json(const json& j) {
		auto& inst = *this;
		Element::from_json(j);
		READREQ(j, path);
		READ(j, fillType);
		READ(j, imageAnchor);
		READ(j, tintColor);
		texture.Load(path);
	}
	void RadarGraph::to_json(json& j) const {
		auto& inst = *this;
		Element::to_json(j);
		WRITE(segments);
		WRITE(groups);
		WRITE(maxValue);
		WRITE(fontSize);
		WRITE(fontColor);
		WRITE(overlapColor);
		WRITE(drawLabels);
		WRITE(drawIcons);
		WRITE(drawOverlap);
	}
	void RadarGraph::from_json(const json& j) {
		auto& inst = *this;
		Element::from_json(j);
		if (segments.size() == 0) { READREQ(j, segments); }
		else { READ(j, segments); }
		READ(j, maxValue);
		READ(j, fontSize);
		READ(j, fontColor);
		READ(j, overlapColor);
		READ(j, drawLabels);
		READ(j, drawIcons);
		READ(j, drawOverlap);

		// Groups with dynamic values support
		if (j.contains("groups") || !j["groups"].is_array() || j["groups"].empty()) {
			groups.clear();
			groups.resize(j["groups"].size());
			int idx = 0;
			for (auto& j_group : j["groups"]) {
				if (j_group.is_object()) {
					auto& group = groups[idx++];
					group.color = j_group.value("color", group.color);
					if (j_group.contains("values")) {
						if (j_group["values"].is_array()) {
							group.values = j_group["values"].get<std::vector<float>>();
						} else if (j_group["values"].is_string()) {
							std::string vals = j_group["values"].get<std::string>();
							static const std::regex pattern{R"(^\{@(\w+)\}$)"};
							std::smatch match;
							if (!std::regex_match(vals, match, pattern)) throw std::runtime_error("RadarGraph group values dynamic string has invalid format");
							dynamic_vars.insert("groups");
							dynamic_vars.insert(std::format("groups[{}]", idx-1));
							layout->registerSharedDataListener(match[1].str(), id);
						} else throw std::runtime_error("RadarGraph group values must be an array or a dynamic string");
					} else throw std::runtime_error("RadarGraph group must contain values");
				} else throw std::runtime_error("RadarGraph group must be an object");
			}
		} else throw std::runtime_error("RadarGraph must contain at least one group");
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
	inline void to_json(nlohmann::json& j, const Dispatch::UI::RadarGraph::Segment& inst) {
		WRITE(label);
		WRITE(icon);
	}
	inline void from_json(const nlohmann::json& j, Dispatch::UI::RadarGraph::Segment& inst) {
		READREQ(j, label);
		READ(j, icon);
	}
	inline void to_json(nlohmann::json& j, const Dispatch::UI::RadarGraph::Group& inst) {
		WRITE(values);
		WRITE(color);
	}
	inline void from_json(const nlohmann::json& j, Dispatch::UI::RadarGraph::Group& inst) {
		READREQ(j, values);
		READ(j, color);
	}
	inline void to_json(nlohmann::json& j, const Dispatch::UI::Button::StatusChanges& inst) {
		WRITE(inner);
		WRITE(outter);
		WRITE(border);
		WRITE(text);
		WRITE(size_mult);
	}
	inline void from_json(const nlohmann::json& j, Dispatch::UI::Button::StatusChanges& inst) {
		READ(j, inner);
		READ(j, outter);
		READ(j, border);
		READ(j, text);
		READ(j, size_mult);
	}
};
