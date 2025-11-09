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
	// static raylib::Font defaultFont{};
	// static raylib::Font emojiFont{"resources/fonts/NotoEmoji-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};
	// static raylib::Font symbolsFont{"resources/fonts/NotoSansSymbols2-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};
	// static raylib::Font fontTitle{"resources/fonts/NotoSans-Bold.ttf", 32};
	// static raylib::Font fontText{"resources/fonts/NotoSans-Regular.ttf", 22};

	// float progress = 0.0f;
	// if (full) {
	// 	const float panelWidth = 800;
	// 	const float panelHeight = 400;
	// 	const raylib::Vector2 panelPos{(GetScreenWidth() - panelWidth) / 2, (GetScreenHeight() - panelHeight) / 2};

	// 	// MAIN BACKGROUND PANEL
	// 	raylib::Rectangle mainRect(panelPos.x, panelPos.y, panelWidth, panelHeight);
	// 	mainRect.Draw(Fade(LIGHTGRAY, 0.5f));
	// 	DrawRectangleLinesEx(mainRect, 2, DARKGRAY);

	// 	// TITLE BAR
	// 	float titleBarHeight = 40;
	// 	DrawRectangle(mainRect.x, mainRect.y, panelWidth, titleBarHeight, BLUE);
	// 	fontTitle.DrawText(name, {mainRect.x + 20, mainRect.y + 6}, 24, 2, WHITE);

	// 	// LEFT PANEL (incident source)
	// 	raylib::Rectangle leftRect(mainRect.x + 10, mainRect.y + 50, 220, 320);
	// 	leftRect.Draw(Fade(DARKGRAY, 0.3f));
	// 	DrawRectangleLinesEx(leftRect, 1, GRAY);
	// 	fontText.DrawText("Caller: Ronaldo", leftRect.GetPosition() + raylib::Vector2{10, 10}, 18, 1, ORANGE);
	// 	fontText.DrawText("\"Someone's robbing the museum...\"", leftRect.GetPosition() + raylib::Vector2{10, 40}, 16, 1, LIGHTGRAY);

	// 	// CENTER PANEL (radar + heroes)
	// 	raylib::Rectangle centerRect(leftRect.x + leftRect.width + 10, leftRect.y, 340, 320);
	// 	centerRect.Draw(Fade(RAYWHITE, 0.4f));
	// 	DrawRectangleLinesEx(centerRect, 1, GRAY);
	// 	fontText.DrawText("Attributes", centerRect.GetPosition() + raylib::Vector2{10, 10}, 18, 1, GRAY);

	// 	// === Radar graph ===
	// 	raylib::Vector2 radarCenter = centerRect.GetPosition() + raylib::Vector2{170, 130};
	// 	float radarRadius = 60;
	// 	DrawPolyLines(radarCenter, 5, radarRadius, 0, GRAY); // pentagon background
	// 	std::tuple<AttrMap<int>, raylib::Color, bool> total{getTotalAttributes(), (raylib::Color)ORANGE, false};
	// 	Utils::drawRadarGraph(radarCenter, radarRadius, {total});

	// 	// Hero portraits
	// 	float heroY = centerRect.y + 220;
	// 	float heroX = centerRect.x + 20;
	// 	for (const auto& hero : assignedHeroes) {
	// 		DrawRectangle(heroX, heroY, 64, 64, Fade(DARKGRAY, 0.5f));
	// 		DrawRectangleLines(heroX, heroY, 64, 64, GRAY);
	// 		fontText.DrawText(hero->name.substr(0, 8), {heroX + 5, heroY + 70}, 14, 1, LIGHTGRAY);
	// 		heroX += 74;
	// 	}

	// 	// RIGHT PANEL (requirements)
	// 	raylib::Rectangle rightRect(centerRect.x + centerRect.width + 10, leftRect.y, 210, 320);
	// 	rightRect.Draw(Fade(DARKGRAY, 0.2f));
	// 	DrawRectangleLinesEx(rightRect, 1, GRAY);
	// 	fontText.DrawText("Requirements", rightRect.GetPosition() + raylib::Vector2{10, 10}, 18, 1, GRAY);

	// 	std::vector<std::string> reqs = {
	// 		"Security system taken over by robbers",
	// 		"Avoid motion sensors",
	// 		"Apprehend the thieves"
	// 	};
	// 	float reqY = rightRect.y + 40;
	// 	for (auto& r : reqs) {
	// 		fontText.DrawText("- " + r, {rightRect.x + 10, reqY}, 16, 1, ORANGE);
	// 		reqY += 22;
	// 	}

	// 	// BUTTONS
	// 	raylib::Rectangle btnStart{mainRect.x + panelWidth - 120, mainRect.y + panelHeight - 40, 100, 28};
	// 	btnStart.Draw(Fade(SKYBLUE, 0.6f));
	// 	DrawRectangleLinesEx(btnStart, 1, BLUE);
	// 	fontText.DrawText("DISPATCH", {btnStart.x + 10, btnStart.y + 5}, 18, 1, WHITE);
	// } else {
	// 	std::string text = "!";
	// 	raylib::Font* font = &defaultFont;
	// 	raylib::Color textColor{RED}, backgroundColor{ORANGE}, timeRemainingColor{LIGHTGRAY}, timeElapsedColor{GRAY};
	// 	switch (status) {
	// 		case PENDING:
	// 			progress = timeElapsed / failureTime;
	// 			break;
	// 		case SELECTED:
	// 			progress = timeElapsed / failureTime;
	// 			textColor = WHITE;
	// 			backgroundColor = SKYBLUE;
	// 			timeElapsedColor = BLUE;
	// 			timeRemainingColor = WHITE;
	// 			break;
	// 		case PROGRESS:
	// 			progress = timeElapsed / missionDuration;
	// 		case TRAVELLING:
	// 			text = "🏃";
	// 			font = &emojiFont;
	// 			textColor = WHITE;
	// 			backgroundColor = SKYBLUE;
	// 			timeElapsedColor = BLUE;
	// 			timeRemainingColor = WHITE;
	// 			break;
	// 		case COMPLETED:
	// 		case FAILED:
	// 			text = "✔";
	// 			font = &symbolsFont;
	// 			textColor = WHITE;
	// 			backgroundColor = ColorLerp(ORANGE, YELLOW, 0.5f);
	// 			timeRemainingColor = DARKGRAY;
	// 			break;
	// 		default:
	// 			textColor = LIGHTGRAY;
	// 			break;
	// 	}
	// 	position.DrawCircle(28, BLACK);
	// 	position.DrawCircle(27, timeRemainingColor);
	// 	DrawCircleSector(position, 27, 0.0f, 360.0f * progress, 180, timeElapsedColor);
	// 	position.DrawCircle(24, BLACK);
	// 	position.DrawCircle(23, backgroundColor);
	// 	raylib::Vector2 textSize = font->MeasureText(text, 36, 2);
	// 	font->DrawText(text, position-textSize/2, 36, 2);
	// }
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
