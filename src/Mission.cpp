#include <Mission.hpp>
#include <Utils.hpp>
#include <cctype>
#include <algorithm>
#include <iostream>
#include <format>

Mission::Mission(
	const std::string& name,
	const std::string& description,
	Vector2 position,
	const std::map<std::string, int> &attr,
	float failureTime,
	float travelDuration,
	float missionDuration
) :
	name(name),
	description(description),
	position(position),
	requiredAttributes{},
	failureTime(failureTime),
	travelDuration(travelDuration),
	missionDuration(missionDuration)
{
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		requiredAttributes[attribute] = value;
	}
};

void Mission::assignHero(const Hero& hero) {
	if (status != SELECTED) return;
	assignedHeroes.push_back(hero);
}

void Mission::unassignHero(const Hero& hero) {
	if (status != SELECTED) return;
	assignedHeroes.erase(std::remove_if(assignedHeroes.begin(), assignedHeroes.end(),
		[&hero](const Hero& h) { return h.name == hero.name; }), assignedHeroes.end());
}

void Mission::unassignHero(const std::string& heroName) {
	if (status != SELECTED) return;
	assignedHeroes.erase(std::remove_if(assignedHeroes.begin(), assignedHeroes.end(),
		[&heroName](const Hero& h) { return h.name == heroName; }), assignedHeroes.end());
}

void Mission::start() {
	if (status != SELECTED) return;
	status = TRAVELLING;
	timeElapsed = 0.0f;
}

void Mission::changeStatus(Status newStatus) {
	status = newStatus;
	timeElapsed = 0.0f;
}

void Mission::update(float deltaTime) {
	timeElapsed += deltaTime;
	switch (status) {
		case PENDING:
			if (timeElapsed >= failureTime) changeStatus(MISSED);
			break;
		case TRAVELLING:
			if (timeElapsed >= travelDuration) changeStatus(PROGRESS);
			break;
		case PROGRESS:
			if (timeElapsed >= missionDuration) changeStatus(isSuccessful() ? COMPLETED : FAILED);
			break;
		default:
			break;
	}
}

void Mission::renderUI(bool full) const {
	if (full) {
		DrawText(("Mission '" + name + "' (Selected)").c_str(), position.x, position.y, 20, BLUE);
		DrawText(("Description: " + description).c_str(), position.x, position.y + 25, 15, LIGHTGRAY);
		DrawText("Required Attributes:", position.x, position.y + 45, 15, LIGHTGRAY);
		int offsetY = 65;
		for (const auto& [attr, value] : requiredAttributes) {
			DrawText(
				std::format(" {} : {}", attr.toString(), value).c_str(),
				position.x, position.y + offsetY, 15, LIGHTGRAY
			);
			offsetY += 20;
		}
	} else {
		Color color;
		switch (status) {
			case PENDING:
				color = YELLOW;
				break;
			case SELECTED:
				color = BLUE;
				break;
			case TRAVELLING:
				color = ORANGE;
				break;
			case PROGRESS:
				color = GREEN;
				break;
			case COMPLETED:
				color = DARKGREEN;
				break;
			case FAILED:
				color = RED;
				break;
			case MISSED:
				color = GRAY;
				break;
			default:
				color = LIGHTGRAY;
				break;
		}
		DrawText(("Mission '" + name + "' here").c_str(), position.x, position.y, 20, color);
	}
}

void Mission::handleInput() {
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		Vector2 mousePos = raylib::Mouse::GetPosition();
		if (abs(mousePos.x - position.x) <= 50 && abs(mousePos.y - position.y) <= 10) {
			if (status == PENDING) status = SELECTED;
		} else {
			if (status == SELECTED) status = PENDING;
		}
	}
}

AttrMap<int> Mission::getTotalAttributes() const {
	AttrMap<int> totalAttributes{};

	for (const auto& hero : assignedHeroes) {
		for (const auto& [attr, value] : hero.attributes) {
			totalAttributes[attr] += value;
		}
	}

	return totalAttributes;
}

int Mission::getSuccessChance() const {
	AttrMap<int> totalAttributes = getTotalAttributes();
	int total = 0;
	int requiredTotal = 0;

	for (const auto& [attr, requiredValue] : requiredAttributes) {
		int heroValue = totalAttributes[attr];
		total += std::min(heroValue, requiredValue);
		requiredTotal += requiredValue;
	}

	if (requiredTotal == 0) return 100;
	return static_cast<int>((static_cast<double>(total) / requiredTotal) * 100);
}

bool Mission::isSuccessful() const {
	int chance = getSuccessChance();
	int roll = (rand() % 100) + 1;
	return roll <= chance;
}
