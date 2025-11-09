#include <cctype>
#include <algorithm>
#include <iostream>
#include <format>
#include <memory>

#include <Utils.hpp>
#include <Mission.hpp>
#include <Hero.hpp>

Mission::Mission(
	const std::string& name,
	const std::string& description,
	raylib::Vector2 position,
	const std::map<std::string, int> &attr,
	int slots,
	float failureTime,
	float travelDuration,
	float missionDuration,
	bool dangerous
) :
	name(name),
	description(description),
	position(position),
	requiredAttributes{},
	slots(slots),
	failureTime(failureTime),
	travelDuration(travelDuration),
	missionDuration(missionDuration),
	dangerous(dangerous)
{
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		requiredAttributes[attribute] = value;
	}
};

void Mission::toggleHero(std::shared_ptr<Hero> hero) {
	bool assigned = assignedHeroes.count(hero);
	if (assigned) unassignHero(hero);
	else assignHero(hero);
}

void Mission::assignHero(std::shared_ptr<Hero> hero) {
	int cnt = static_cast<int>(assignedHeroes.size());
	if (cnt < slots && hero->status == Hero::AVAILABLE) {
		assignedHeroes.insert(hero);
		hero->changeStatus(Hero::ASSIGNED, weak_from_this());
	}
}

void Mission::unassignHero(std::shared_ptr<Hero> hero) {
	if (assignedHeroes.count(hero)) {
		assignedHeroes.erase(hero);
		hero->changeStatus(Hero::AVAILABLE);
	}
}

void Mission::changeStatus(Status newStatus) {
	Status oldStatus = status;
	status = newStatus;
	if (newStatus != PENDING && newStatus != SELECTED) timeElapsed = 0.0f;

	if (oldStatus == SELECTED && newStatus == PENDING) for (auto hero : assignedHeroes) unassignHero(hero);
	if (newStatus == TRAVELLING) for (auto hero : assignedHeroes) hero->changeStatus(Hero::TRAVELLING, travelDuration);
	if (oldStatus == PROGRESS) for (auto hero : assignedHeroes) {
		std::cout << "Mission " << name << " completed, it was a " << (newStatus==COMPLETED ? "success" : "failure") << std::endl;
		hero->changeStatus(Hero::RETURNING, travelDuration);
		if (newStatus == COMPLETED) {
			// TODO:
			// int exp = 1000 / std::sqrt(getSuccessChance() || 0.001f);
			// for (auto hero : assignedHeroes) hero.addExp(exp);
		} else if (newStatus == FAILED && dangerous) {
			auto hero = Utils::random_element(assignedHeroes);
			hero->wound();
			std::cout << hero->name << " was wounded" << std::endl;
		}
	}
}

void Mission::update(float deltaTime) {
	int working = std::count_if(assignedHeroes.begin(), assignedHeroes.end(), [&](auto hero){ return hero->status == Hero::WORKING; });
	switch (status) {
		case PENDING:
			timeElapsed += deltaTime;
			if (timeElapsed >= failureTime) changeStatus(MISSED);
			break;
		case TRAVELLING:
			if (working > 0) changeStatus(PROGRESS);
			break;
		case PROGRESS:
			timeElapsed += deltaTime * (working / assignedHeroes.size());
			if (timeElapsed >= missionDuration) changeStatus(isSuccessful() ? COMPLETED : FAILED);
			break;
		case SELECTED:
			break;
		default:
			timeElapsed += deltaTime;
			break;
	}
}

void Mission::renderUI(bool full) const {
	static raylib::Font defaultFont{};
	static raylib::Font emojiFont{"resources/fonts/NotoEmoji-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};
	static raylib::Font symbolsFont{"resources/fonts/NotoSansSymbols2-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};

	float progress = 0.0f;
	if (full) {
		raylib::Rectangle rect = {170, 100, 550, 240};
		raylib::Vector2 start = rect.GetPosition();
		rect.Draw(Fade(DARKGRAY, 0.9f));

		raylib::Vector2 offset = {100, 10};
		defaultFont.DrawText("Mission '" + name + "' (Selected)", start + offset, 20, 2, BLUE); offset.y += 25;
		defaultFont.DrawText("Description: " + description, start + offset, 20, 2, LIGHTGRAY); offset.y += 20;

		offset = raylib::Vector2{20, 80};
		defaultFont.DrawText("Assigned Heroes:", start + offset, 20, 2, LIGHTGRAY); offset.y += 20;
		for (const auto& hero : assignedHeroes) {
			defaultFont.DrawText(" - " + hero->name, start + offset, 15, 2, LIGHTGRAY); offset.y += 20;
		}
		for (int i = static_cast<int>(assignedHeroes.size()); i < slots; ++i) {
			defaultFont.DrawText(" - [Empty Slot]", start + offset, 15, 2, GRAY); offset.y += 20;
		}

		offset = raylib::Vector2{170, 80};
		defaultFont.DrawText("Required Attributes:", start + offset, 15, 2, LIGHTGRAY); offset.y += 20;
		for (const auto& [attr, value] : requiredAttributes) {
			defaultFont.DrawText(std::format(" {} : {}", attr.toString(), value), start + offset, 15, 2, LIGHTGRAY); offset.y += 20;
		}

		offset = raylib::Vector2{320, 80};
		defaultFont.DrawText("Attributes from Heroes:", start + offset, 15, 2, LIGHTGRAY); offset.y += 20;
		for (const auto& [attr, value] : getTotalAttributes()) {
			defaultFont.DrawText(std::format(" {} : {}", attr.toString(), value), start + offset, 15, 2, LIGHTGRAY); offset.y += 20;
		}

		raylib::Color color = assignedHeroes.size() > 0 ? LIGHTGRAY : ColorAlpha(LIGHTGRAY, 0.3f);
		raylib::Vector2 textSize = defaultFont.MeasureText("Cancel", 15, 2);
		rect = raylib::Rectangle{540, 305, 80, 30};

		rect.DrawLines(LIGHTGRAY);
		defaultFont.DrawText("Cancel", rect.GetPosition() + rect.GetSize()/2 - textSize/2, 15, 2, LIGHTGRAY);

		rect.x += 95;
		textSize = defaultFont.MeasureText("Start", 15, 2);
		rect.DrawLines(LIGHTGRAY);
		defaultFont.DrawText("Cancel", rect.GetPosition() + rect.GetSize()/2 - textSize/2, 15, 2, color);
	} else {
		std::string text = "!";
		raylib::Font* font = &defaultFont;
		raylib::Color textColor{RED}, backgroundColor{ORANGE}, timeRemainingColor{LIGHTGRAY}, timeElapsedColor{GRAY};
		switch (status) {
			case PENDING:
				progress = timeElapsed / failureTime;
				break;
			case SELECTED:
				progress = timeElapsed / failureTime;
				textColor = WHITE;
				backgroundColor = SKYBLUE;
				timeElapsedColor = BLUE;
				timeRemainingColor = WHITE;
				break;
			case PROGRESS:
				progress = timeElapsed / missionDuration;
			case TRAVELLING:
				text = "🏃";
				font = &emojiFont;
				textColor = WHITE;
				backgroundColor = SKYBLUE;
				timeElapsedColor = BLUE;
				timeRemainingColor = WHITE;
				break;
			case COMPLETED:
			case FAILED:
				text = "✔";
				font = &symbolsFont;
				textColor = WHITE;
				backgroundColor = ColorLerp(ORANGE, YELLOW, 0.5f);
				timeRemainingColor = DARKGRAY;
				break;
			default:
				textColor = LIGHTGRAY;
				break;
		}
		position.DrawCircle(28, BLACK);
		position.DrawCircle(27, timeRemainingColor);
		DrawCircleSector(position, 27, 0.0f, 360.0f * progress, 180, timeElapsedColor);
		position.DrawCircle(24, BLACK);
		position.DrawCircle(23, backgroundColor);
		raylib::Vector2 textSize = font->MeasureText(text, 36, 2);
		font->DrawText(text, position-textSize/2, 36, 2);
	}
}

void Mission::handleInput() {
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		if (status == PENDING) {
			if (abs(mousePos.x - position.x) <= 28 && abs(mousePos.y - position.y) <= 28) status = SELECTED;
		} else {
			if (status == SELECTED) {
				if (mousePos.x >= 635 && mousePos.x <= 715 && mousePos.y >= 305 && mousePos.y <= 335 && assignedHeroes.size() > 0) changeStatus(TRAVELLING);
				else if (mousePos.x >= 540 && mousePos.x <= 620 && mousePos.y >= 305 && mousePos.y <= 335) changeStatus(PENDING);
			}
		}
	}
}

AttrMap<int> Mission::getTotalAttributes() const {
	AttrMap<int> totalAttributes{};

	for (const auto& hero : assignedHeroes) for (const auto& [attr, value] : hero->attributes()) totalAttributes[attr] += value;

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
