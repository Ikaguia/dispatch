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
	int slots,
	float failureTime,
	float travelDuration,
	float missionDuration
) :
	name(name),
	description(description),
	position(position),
	requiredAttributes{},
	slots(slots),
	failureTime(failureTime),
	travelDuration(travelDuration),
	missionDuration(missionDuration)
{
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		requiredAttributes[attribute] = value;
	}
};

void Mission::toggleHero(std::shared_ptr<Hero> hero) {
	if (status != SELECTED) return;
	bool assigned = assignedHeroes.count(hero);
	int cnt = static_cast<int>(assignedHeroes.size());
	if (assigned) assignedHeroes.erase(hero);
	else if (cnt < slots) assignedHeroes.insert(hero);
}

void Mission::assignHero(std::shared_ptr<Hero> hero) {
	if (status != SELECTED) return;
	int cnt = static_cast<int>(assignedHeroes.size());
	if (cnt < slots) assignedHeroes.insert(hero);
}

void Mission::unassignHero(std::shared_ptr<Hero> hero) {
	if (status != SELECTED) return;
	assignedHeroes.erase(hero);
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
		Vector2 start = {170, 100};
		Vector2 size = {550, 220};
		DrawRectangle(start.x, start.y, size.x, size.y, Fade(DARKGRAY, 0.9f));

		Vector2 offset = {100, 10};
		DrawText(("Mission '" + name + "' (Selected)").c_str(), start.x + offset.x, start.y + offset.y, 20, BLUE); offset.y += 25;
		DrawText(("Description: " + description).c_str(), start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY); offset.y += 20;

		offset = {20, 80};
		DrawText("Assigned Heroes:", start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY); offset.y += 20;
		for (const auto& hero : assignedHeroes) {
			DrawText(
				std::format(" - {}", hero->name).c_str(),
				start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY
			);
			offset.y += 20;
		}
		for (int i = static_cast<int>(assignedHeroes.size()); i < slots; ++i) {
			DrawText(
				" - [Empty Slot]",
				start.x + offset.x, start.y + offset.y, 15, GRAY
			);
			offset.y += 20;
		}

		offset = {170, 80};
		DrawText("Required Attributes:", start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY); offset.y += 20;
		for (const auto& [attr, value] : requiredAttributes) {
			DrawText(
				std::format(" {} : {}", attr.toString(), value).c_str(),
				start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY
			);
			offset.y += 20;
		}

		offset = {320, 80};
		DrawText("Total Attributes from Heroes:", start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY); offset.y += 20;
		for (const auto& [attr, value] : getTotalAttributes()) {
			DrawText(
				std::format(" {} : {}", attr.toString(), value).c_str(),
				start.x + offset.x, start.y + offset.y, 15, LIGHTGRAY
			);
			offset.y += 20;
		}

		DrawRectangleLines(540, 285, 80, 30, LIGHTGRAY);
		DrawText("Cancel", 555, 295, 15, LIGHTGRAY);

		DrawRectangleLines(635, 285, 80, 30, LIGHTGRAY);
		DrawText("Start", 650, 295, 15, LIGHTGRAY);
	} else {
		Color color;
		switch (status) {
			case PENDING:
				color = ColorLerp(YELLOW, GRAY, timeElapsed / failureTime);
				break;
			case SELECTED:
				color = BLUE;
				break;
			case TRAVELLING:
				color = ColorLerp(BLUE, GREEN, timeElapsed / failureTime);
				break;
			case PROGRESS:
				color = ColorLerp(GREEN, DARKGREEN, timeElapsed / failureTime);
				break;
			case COMPLETED:
				color = ColorAlpha(DARKGREEN, (5.f - (timeElapsed > 5.0f ? timeElapsed-5.0f : 0.0f)) / 5.0f);
				break;
			case FAILED:
				color = ColorAlpha(RED, (5.f - (timeElapsed > 5.0f ? timeElapsed-5.0f : 0.0f)) / 5.0f);
				break;
			case MISSED:
				color = ColorAlpha(GRAY, (5.f - (timeElapsed > 5.0f ? timeElapsed-5.0f : 0.0f)) / 5.0f);
				break;
			default:
				color = LIGHTGRAY;
				break;
		}
		DrawText("!", position.x, position.y, 50, color);
		DrawRectangleLines(position.x-15, position.y-5, 35, 50, BLACK);
	}
}

void Mission::handleInput() {
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		Vector2 mousePos = raylib::Mouse::GetPosition();
		if (status == PENDING) {
			if (mousePos.x >= (position.x-15) && mousePos.x <= (position.x+20) && mousePos.y >= (position.y-5) && mousePos.y <= (position.y+45) ) status = SELECTED;
		} else {
			if (status == SELECTED) {
				if (mousePos.x >= 640 && mousePos.x <= 720 && mousePos.y >= 270 && mousePos.y <= 320) changeStatus(TRAVELLING);
				else if (mousePos.x >= 540 && mousePos.x <= 620 && mousePos.y >= 270 && mousePos.y <= 320) changeStatus(PENDING);
			}
		}
	}
}

AttrMap<int> Mission::getTotalAttributes() const {
	AttrMap<int> totalAttributes{};

	for (const auto& hero : assignedHeroes) for (const auto& [attr, value] : hero->attributes) totalAttributes[attr] += value;

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
