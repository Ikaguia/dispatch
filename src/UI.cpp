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
		else if (type == "SCROLLBOX") return std::make_unique<ScrollBox>();
		else if (type == "DATAINSPECTOR") return std::make_unique<DataInspector>();
		else throw std::invalid_argument("Invalid type in JSON layout");
	}

	// Layout
	Layout::Layout(std::string path) {
		json data_array = Utils::readJsonFile(path);
		Utils::println("Reading layout '{}'", path);

		if (!data_array.is_array()) throw std::runtime_error("Layout Error: Top-level JSON must be an array of elements.");

		for (const auto& data : data_array) {
			std::string type = data.at("type").get<std::string>();
			if (type != "STYLE") continue;
			auto style = std::make_unique<Style>();
			try { data.get_to(*style); }
			catch (const std::exception& e) { throw std::runtime_error(std::format("Layout Error: Failed to deserialize style. {}", e.what())); }
			if (styles.count(style->id)) throw std::runtime_error("Layout Error: Style with ID '" + style->id + "' already exists in the map.");

			std::cout << "Read style " << style->id << std::endl;
			styles[style->id] = std::move(style);
		}

		for (const auto& data : data_array) {
			std::string type = data.at("type").get<std::string>();
			if (type == "STYLE") continue;
			auto el = elementFactory(type);
			el->layout = this;
			try { data.get_to(*el); }
			catch (const std::exception& e) { throw std::runtime_error("Layout Error: Failed to deserialize element of type '" + type + "'. " + e.what()); }
			if (elements.count(el->id)) throw std::runtime_error("Layout Error: Element with ID '" + el->id + "' already exists in the map.");
			if (el->father_id.empty()) rootElements.insert(el->id);

			std::cout << "Read element " << el->id << std::endl;
			elements[el->id] = std::move(el);
		}

		for (auto& [id, _] : elements) {
			auto& el = *elements[id].get();
			if (!el.father_id.empty() && !elements.count(el.father_id)) {
				throw std::runtime_error(std::format("Layout Error: Father id '{}' for element '{}' is not present in the layout", el.father_id, el.id));
			}
			for (const auto& subElement_id : el.subElement_ids) {
				if (elements.count(subElement_id)) {
					if (elements[subElement_id]->father_id != el.id) {
						throw std::runtime_error(std::format("Layout Error: Father/Subelements mismatch between '{}' and '{}'", el.id, subElement_id));
					}
				} else throw std::runtime_error(std::format("Layout Error: Subelement id '{}' for element '{}' is not present in the layout", subElement_id, el.id));
			}

		}

		if (!data_array.empty() && rootElements.empty()) throw std::runtime_error("Layout Error: No root element found");
		for (const std::string& id : rootElements) elements.at(id)->solveLayout();
	}
	void Layout::sync() {
		if (needsRebuild.empty()) return;
		for (const std::string& id : needsRebuild) if (elements.contains(id)) elements.at(id)->rebuild();
		needsRebuild.clear();
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
		sync();
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
	void Layout::unregisterSharedDataListener(const std::string& key, const std::string& element_id) {
		Utils::println("Unregistering element '{}' as listener for shared data key '{}'", element_id, key);
		sharedDataListeners[key].erase(element_id);
	}
	void Layout::removeElement(const std::string& id, const std::string& loop) {
		if (loop != "sharedDataListeners") for (auto& [key, mp] : sharedDataListeners) mp.erase(id);
		if (loop != "rootElements") rootElements.erase(id);
		if (loop != "hovered") hovered.erase(id);
		if (loop != "clicked") clicked.erase(id);
		if (loop != "pressed") pressed.erase(id);
		if (loop != "unhover") unhover.erase(id);
		if (loop != "release") release.erase(id);
		if (loop != "dataChanged") dataChanged.erase(id);
		if (loop != "needsRebuild") needsRebuild.erase(id);
		if (loop != "elements") elements.erase(id);
	}

	// Element
	const float Element::side(Side side, bool inner) const {
		const auto& rect = inner ? innerRect() : outterRect();
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
	raylib::Vector2 Element::anchor(Utils::Anchor anchor) const { return Utils::anchorPos(outterRect(), anchor); };
	const raylib::Rectangle& Element::outterRect() const {
		if (!initialized) throw std::runtime_error("Element bounding rect requested before initialization");
		return bounds;
	}
	const raylib::Rectangle& Element::innerRect() const {
		if (!initialized) throw std::runtime_error("Element inner bounding rect requested before initialization");
		return innerBounds;
	}
	const bool Element::colidesWith(const Element& other) const { return outterRect().CheckCollision(other.outterRect()); }
	const bool Element::colidesWith(const raylib::Vector2& other) const { return outterRect().CheckCollision(other); }
	const bool Element::colidesWith(const raylib::Rectangle& other) const { return outterRect().CheckCollision(other); }
	void Element::render() {
		if (!visible) return;
		_render();
		auto& elements = layout->elements;
		for (const std::string& el_id : subElement_ids) elements.at(el_id)->render();
	}
	void Element::_render() {
		raylib::Rectangle outterRect = this->outterRect();
		raylib::Rectangle innerRect = this->innerRect();
		
		if ((shadowOffset.x != 0 || shadowOffset.y != 0) && shadowColor.a != 0) {
			raylib::Rectangle shadowRect{outterRect.GetPosition() + shadowOffset, outterRect.GetSize()};
			if (roundnessSegments > 0 && roundness != 1.0f) shadowRect.DrawRounded(roundness, roundnessSegments, shadowColor);
			else shadowRect.Draw(shadowColor);
		}
		if (roundnessSegments > 0 && roundness != 1.0f) {
			outterRect.DrawRounded(roundness, roundnessSegments, outterColors[0]);
			innerRect.DrawRounded(roundness, roundnessSegments, innerColors[0]);
		} else {
			int outterColorsSize = (int)outterColors.size();
			int innerColorsSize = (int)innerColors.size();
			if (outterColorsSize == 1) outterRect.Draw(outterColors[0]);
			else if (outterColorsSize == 2) outterRect.DrawGradientH(outterColors[0], outterColors[1]);
			else if (outterColorsSize == 4) outterRect.DrawGradient(outterColors[0], outterColors[1], outterColors[2], outterColors[3]);
			if (innerColorsSize == 1) innerRect.Draw(innerColors[0]);
			else if (innerColorsSize == 2) innerRect.DrawGradientH(innerColors[0], innerColors[1]);
			else if (innerColorsSize == 4) innerRect.DrawGradient(innerColors[0], innerColors[1], innerColors[2], innerColors[3]);
		}
		if (borderThickness > 0.0f) {
			if (roundnessSegments > 0 && roundness != 1.0f) outterRect.DrawRoundedLines(roundness, roundnessSegments, borderThickness, borderColor);
			else outterRect.DrawLines(borderColor, borderThickness);
		}
	}
	void Element::handleInput(raylib::Vector2 offset) {
		for (const std::string& el_id : subElement_ids) layout->elements.at(el_id)->handleInput(offset);
		_handleInput(offset);
	}
	void Element::_handleInput(raylib::Vector2 offset) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition() + offset;
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
			float maxW = father_id.empty() ? GetScreenWidth() : elements.at(father_id)->innerRect().width;
			size.x = std::round(maxW * origSize.x - (outterMargin.x + outterMargin.z));
		}
		if (origSize.y >= 0.0f && origSize.y <= 1.0f) {
			float maxH = father_id.empty() ? GetScreenHeight() : elements.at(father_id)->innerRect().height;
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
	void Element::applyStyles(bool force) {
		for (const std::string& style_id : style_ids) applyStyle(style_id, force);
	}
	void Element::applyStyle(const std::string& st_id, bool force) {
		if (st_id.empty()) return;
		if (!layout->styles.contains(st_id)) return;
		Style* style = layout->styles.at(st_id).get();
		if (!style->father_id.empty()) applyStyle(style->father_id, force);
		for (auto& [key, value] : style->orig.items()) {
			if (!force && orig.contains(key)) continue;
			applyStylePart(key, value);
		}
	}
	bool Element::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (key == "borderThickness") value.get_to(borderThickness);
		else if (key == "roundness") value.get_to(roundness);
		else if (key == "size") value.get_to(size);
		else if (key == "shadowOffset") value.get_to(shadowOffset);
		else if (key == "outterMargin") value.get_to(outterMargin);
		else if (key == "innerMargin") value.get_to(innerMargin);
		else if (key == "innerColors") value.get_to(innerColors);
		else if (key == "outterColors") value.get_to(outterColors);
		else if (key == "borderColor") value.get_to(borderColor);
		else if (key == "shadowColor") value.get_to(shadowColor);
		else if (key == "shadow") value.get_to(shadow);
		else if (key == "verticalConstraint") value.get_to(verticalConstraint);
		else if (key == "verticalConstraint.offset") value.get_to(verticalConstraint.offset);
		else if (key == "verticalConstraint.ratio") value.get_to(verticalConstraint.ratio);
		else if (key == "verticalConstraint.start") value.get_to(verticalConstraint.start);
		else if (key == "verticalConstraint.start.type") value.get_to(verticalConstraint.start.type);
		else if (key == "verticalConstraint.start.side") value.get_to(verticalConstraint.start.side);
		else if (key == "verticalConstraint.end") value.get_to(verticalConstraint.end);
		else if (key == "verticalConstraint.end.type") value.get_to(verticalConstraint.end.type);
		else if (key == "verticalConstraint.end.side") value.get_to(verticalConstraint.end.side);
		else if (key == "horizontalConstraint") value.get_to(horizontalConstraint);
		else if (key == "horizontalConstraint.offset") value.get_to(horizontalConstraint.offset);
		else if (key == "horizontalConstraint.ratio") value.get_to(horizontalConstraint.ratio);
		else if (key == "horizontalConstraint.start") value.get_to(horizontalConstraint.start);
		else if (key == "horizontalConstraint.start.type") value.get_to(horizontalConstraint.start.type);
		else if (key == "horizontalConstraint.start.side") value.get_to(horizontalConstraint.start.side);
		else if (key == "horizontalConstraint.end") value.get_to(horizontalConstraint.end);
		else if (key == "horizontalConstraint.end.type") value.get_to(horizontalConstraint.end.type);
		else if (key == "horizontalConstraint.end.side") value.get_to(horizontalConstraint.end.side);
		else return false;
		return true;
	}
	// Text
	void Text::_render() {
		Element::_render();
		raylib::Rectangle rect = innerRect();
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
	bool Text::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (Element::applyStylePart(key, value)) ;
		else if (key == "fontSize") value.get_to(fontSize);
		else if (key == "spacing") value.get_to(spacing);
		else if (key == "text") value.get_to(text);
		else if (key == "fontName") value.get_to(fontName);
		else if (key == "fontColor") value.get_to(fontColor);
		else if (key == "textAnchor") value.get_to(textAnchor);
		else return false;
		return true;
	}
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
			outterRect(),
			tintColor,
			fillType,
			imageAnchor
		);
	}
	bool Image::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (Element::applyStylePart(key, value)) ;
		else if (key == "path") value.get_to(path);
		else if (key == "fillType") value.get_to(fillType);
		else if (key == "imageAnchor") value.get_to(imageAnchor);
		else if (key == "tintColor") value.get_to(tintColor);
		else return false;
		return true;
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
		float oldSizeMult = size_mult;
		Element::changeStatus(st);
		if (force || (status != oldStatus)) {
			StatusChanges& sc = statusChanges[status];
			innerColors = sc.inner;
			outterColors = sc.outter;
			borderColor = sc.border;
			fontColor = sc.text;
			size_mult = sc.size_mult;
			if (initialized && (size_mult != oldSizeMult)) solveLayout();
		}
	}
	bool Button::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (TextBox::applyStylePart(key, value)) ;
		else if (key == "statusChanges") value.get_to(statusChanges);
		else if (key == "size_mult") value.get_to(size_mult);
		else {
			for (Status st : statuses) {
				if (key == std::format("statusChanges.{}", json{st}.dump())) {
					value.get_to(statusChanges[st]);
					return true;
				} else if (key == std::format("statusChanges.{}.inner", json{st}.dump())) {
					value.get_to(statusChanges[st].inner);
					return true;
				} else if (key == std::format("statusChanges.{}.outter", json{st}.dump())) {
					value.get_to(statusChanges[st].outter);
					return true;
				} else if (key == std::format("statusChanges.{}.border", json{st}.dump())) {
					value.get_to(statusChanges[st].border);
					return true;
				} else if (key == std::format("statusChanges.{}.text", json{st}.dump())) {
					value.get_to(statusChanges[st].text);
					return true;
				} else if (key == std::format("statusChanges.{}.size_mult", json{st}.dump())) {
					value.get_to(statusChanges[st].size_mult);
					return true;
				}
			}
			return false;
		}
		return true;
	}
	// RadioButton
	void RadioButton::_handleInput(raylib::Vector2 offset) {
		Button::_handleInput(offset);
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
		raylib::Rectangle rect = innerRect();
		raylib::Vector2 center = this->center();
		float sideLength = std::min(rect.width, rect.height) / 2.0f;
		if (drawIcons) sideLength *= 0.8f;

		const int sides = segments.size();
		float baseRotation = 90.0f + 180.0f / sides;
		DrawPoly(center, sides, sideLength+1, baseRotation, innerColors[0]);
		DrawPolyLines(center, sides, sideLength-1, baseRotation, borderColor);
		for (int i = 1; i < 5; i++) DrawPolyLines(center, sides, i * sideLength / 5, baseRotation, bgLines);
		for (int i = 0; i < sides; i++) center.DrawLine(center + raylib::Vector2{0, -sideLength}.Rotate(i * 2.0f * PI / sides), bgLines);
		if (drawIcons) for (int i = 0; i < sides; i++) {
			std::string icon = segments[i].icon;
			auto pos = center + raylib::Vector2{0.0f, -(sideLength * 1.33f)}.Rotate(i * 2.0f * PI / sides);
			if ((int)innerColors.size() == sides) pos.DrawCircle(18, innerColors[i]);
			else pos.DrawCircle(18, innerColors[0]);
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
	bool RadarGraph::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (Element::applyStylePart(key, value)) ;
		else if (key == "segments") value.get_to(segments);
		else if (key == "groups") value.get_to(groups);
		else if (key == "maxValue") value.get_to(maxValue);
		else if (key == "fontSize") value.get_to(fontSize);
		else if (key == "precision") value.get_to(precision);
		else if (key == "fontColor") value.get_to(fontColor);
		else if (key == "overlapColor") value.get_to(overlapColor);
		else if (key == "bgLines") value.get_to(bgLines);
		else if (key == "drawLabels") value.get_to(drawLabels);
		else if (key == "drawIcons") value.get_to(drawIcons);
		else if (key == "drawOverlap") value.get_to(drawOverlap);
		else return false;
		return true;
	}
	// ScrollBox
	void ScrollBox::render() {
		raylib::Rectangle viewport = innerRect();

		if (!visible) return;
		_render();
		BeginScissorMode((int)viewport.x, (int)viewport.y, (int)viewport.width, (int)viewport.height);
			rlPushMatrix();
			rlTranslatef(scrollOffset.x, scrollOffset.y, 0);
				auto& elements = layout->elements;
				for (const std::string& el_id : subElement_ids) elements.at(el_id)->render();
			rlPopMatrix();
		EndScissorMode();
	}
	void ScrollBox::handleInput(raylib::Vector2 offset) {
		auto& elements = layout->elements;
		for (const std::string& el_id : subElement_ids) elements.at(el_id)->handleInput(offset + scrollOffset);
		_handleInput(offset);
	}
	void ScrollBox::_handleInput(raylib::Vector2 offset) {
		Box::_handleInput();

		if (hovered) {
			raylib::Vector2 wheel = raylib::Mouse::GetWheelMoveV();
			if (wheel != raylib::Vector2{0.0f, 0.0f}) {
				if (scrollX) scrollOffset.x += wheel.x * 20.0f;
				if (scrollY) scrollOffset.y += wheel.y * 20.0f;

				float maxScrollX = contentSize.x - innerRect().width;
				scrollOffset.x = std::clamp(scrollOffset.x, -maxScrollX, 0.0f);

				float maxScrollY = contentSize.y - innerRect().height;
				scrollOffset.y = std::clamp(scrollOffset.y, -maxScrollY, 0.0f);
			}
		}
	}
	void ScrollBox::solveLayout() {
		Element::solveLayout();
		contentSize = raylib::Vector2{};
		for (std::string id : subElement_ids) {
			Element* el = layout->elements[id].get();
			contentSize.x = std::max(contentSize.x, el->side(Side::END));
			contentSize.y = std::max(contentSize.y, el->side(Side::BOTTOM));
		}
	}
	bool ScrollBox::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (Box::applyStylePart(key, value)) ;
		else if (key == "scrollX") value.get_to(scrollX);
		else if (key == "scrollY") value.get_to(scrollY);
		else return false;
		return true;
	}
	// DataInspector
	void DataInspector::onSharedDataUpdate(const std::string& key, const nlohmann::json& value) {
		if (key == dataPath) layout->needsRebuild.insert(id);
	}
	void DataInspector::rebuild() {
		json& data = layout->sharedData[dataPath];

		for (const std::string& child_id : subElement_ids) {
			if (!fixedChilds.contains(child_id)) {
				layout->removeElement(child_id, "needsRebuild");
			}
		}
		subElement_ids.assign(fixedChilds.begin(), fixedChilds.end());

		std::string last_id = "";
		float bottom = -1;

		for (const std::string& child_id : subElement_ids) {
			Element* child = layout->elements.at(child_id).get();
			float c_bottom = child->side(Side::BOTTOM);
			if (last_id.empty() || c_bottom > bottom) {
				last_id = child_id;
				bottom = c_bottom;
			}
		}

		if (data.is_object() || data.is_array()) {
			for (auto& [key, value] : data.items()) {
				std::string row_id = std::format("{}_row_{}", id, key), text;
				if (data.is_object()) {
					text = std::format("{}: {}", key, value.is_string() ? value.get<std::string>() : value.dump());
				} else {
					text = std::format("{}", value.is_string() ? value.get<std::string>() : value.dump());
				}

				raylib::Vector4 innerMargin{5, 2, 5, 2};
				json tb_json = {
					{"type", "TEXTBOX"},
					{"id", row_id},
					{"father_id", id},
					{"text", text},
					{"fontSize", valueFontSize},
					{"innerMargin", innerMargin},
					{"size", {1.0, 1.0}},
					{"textAnchor", "left"}
				};

				float availableWidth = this->innerRect().width - (innerMargin.x + innerMargin.z);

				raylib::Rectangle textBounds = Utils::positionTextAnchored(
					tb_json["text"], 
					raylib::Rectangle(0, 0, availableWidth, 1000),
					Utils::Anchor::topLeft,
					defaultFont,
					tb_json["fontSize"].get<float>(),
					1.0f,
					{0, 0},
					availableWidth
				);

				if (orientation == Orientation::VERTICAL) {
					tb_json["horizontalConstraint"] = {
						{"start", {{"type", "father"}, {"side", "start"}}},
						{"end", {{"type", "father"}, {"side", "end"}}}
					};
					tb_json["verticalConstraint"] = {{"offset", itemSpacing}};
					if (last_id.empty()) tb_json["verticalConstraint"]["start"] = {{"type", "father"}, {"side", "top"}};
					else tb_json["verticalConstraint"]["start"] = {{"type", "element"}, {"element_id", last_id}, {"side", "bottom"}};

					tb_json["size"][1] = textBounds.height + (innerMargin.y + innerMargin.w);
				} else {
					tb_json["verticalConstraint"] = {
						{"start", {{"type", "father"}, {"side", "top"}}},
						{"end", {{"type", "father"}, {"side", "bottom"}}}
					};
					tb_json["horizontalConstraint"] = {{"offset", itemSpacing}};
					if (last_id.empty()) tb_json["horizontalConstraint"]["start"] = {{"type", "father"}, {"side", "start"}};
					else tb_json["horizontalConstraint"]["start"] = {{"type", "element"}, {"element_id", last_id}, {"side", "end"}};

					tb_json["size"][0] = textBounds.width + (innerMargin.x + innerMargin.z);
				}

				auto tb = std::make_unique<TextBox>();
				tb->layout = layout;
				tb_json.get_to(*tb);

				layout->elements[row_id] = std::move(tb);
				subElement_ids.push_back(row_id);
				last_id = row_id;
			}
		} else return;

		solveLayout();
	}
	bool DataInspector::applyStylePart(const std::string& key, const nlohmann::json& value) {
		if (ScrollBox::applyStylePart(key, value)) ;
		else if (key == "labelFontSize") value.get_to(labelFontSize);
		else if (key == "valueFontSize") value.get_to(valueFontSize);
		else if (key == "itemSpacing") value.get_to(itemSpacing);
		else if (key == "orientation") value.get_to(orientation);
		else return false;
		return true;
	}

	// JSON Serialization Macros
	#define READ(j, var)		inst.var = j.value(#var, inst.var)
	#define READREQ(j, var) { \
								if (j.contains(#var)) j.at(#var).get_to(inst.var); \
								else throw std::invalid_argument("Hero '" + std::string(#var) + "' cannot be empty"); \
							}
	#define READ2(j, var, key)	inst.var = j.value(#key, inst.var)
	#define READREQ2(j, var, key) { \
								if (j.contains(#key)) j.at(#key).get_to(inst.var); \
								else throw std::invalid_argument("Hero '" + std::string(#key) + "' cannot be empty"); \
							}
	#define WRITE(var)			j[#var] = inst.var;
	#define WRITE2(var, key)	j[#key] = inst.var;

	// JSON Serialization
	void Style::to_json(json& j) const {
		j = orig;
	}
	void Style::from_json(const json& j) {
		orig = j;
		auto& inst = *this;
		READ(j, id);
	}
	void Element::to_json(json& j) const {
		auto& inst = *this;
		WRITE(style_ids);
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
		WRITE(innerColors);
		WRITE(outterColors);
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
		READ(j, innerColors);
		READ(j, outterColors);
		if (j.contains("innerColor")) {
			raylib::Color innerColor = j["innerColor"].get<raylib::Color>();
			innerColors = {innerColor};
		}
		if (j.contains("outterColor")) {
			raylib::Color outterColor = j["outterColor"].get<raylib::Color>();
			outterColors = {outterColor};
		}
		READ(j, borderColor);
		READ(j, shadowColor);
		READREQ(j, verticalConstraint);
		READREQ(j, horizontalConstraint);
		READ(j, style_ids);
		applyStyles();
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
	void ScrollBox::to_json(nlohmann::json& j) const {
		auto& inst = *this;
		Box::to_json(j);
		WRITE(scrollX);
		WRITE(scrollY);
	}
	void ScrollBox::from_json(const nlohmann::json& j) {
		auto& inst = *this;
		Box::from_json(j);
		READ(j, scrollX);
		READ(j, scrollY);
	}
	void DataInspector::from_json(const nlohmann::json& j) {
		auto& inst = *this;
		READREQ(j, dataPath);
		READ(j, labelFontSize);
		READ(j, valueFontSize);
		READ(j, itemSpacing);
		READ(j, orientation);
		if (orientation == Orientation::HORIZONTAL) {
			scrollX = true;
			scrollY = false;
		}
		ScrollBox::from_json(j);

		fixedChilds.clear();
		fixedChilds = std::set<std::string>{subElement_ids.begin(), subElement_ids.end()};
		layout->registerSharedDataListener(dataPath, id);
		if (layout->sharedData.contains(dataPath)) rebuild();
	}
	void DataInspector::to_json(nlohmann::json& j) const {
		auto& inst = *this;
		ScrollBox::to_json(j);
		WRITE(dataPath);
		WRITE(labelFontSize);
		WRITE(valueFontSize);
		WRITE(itemSpacing);
		WRITE(orientation);
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
