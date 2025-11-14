#include <cctype>
#include <algorithm>
#include <iostream>
#include <format>
#include <memory>

#include <Utils.hpp>
#include <Mission.hpp>
#include <Hero.hpp>
#include <MissionsHandler.hpp>

Mission::Mission(
	const std::string& name,
	const std::string& description,
	raylib::Vector2 position,
	const std::map<std::string, int> &attr,
	int slots,
	float failureTime,
	float missionDuration,
	bool dangerous
) :
	name(name),
	description(description),
	position(position),
	requiredAttributes{},
	slots(slots),
	failureTime(failureTime),
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
		hero->changeStatus(Hero::AVAILABLE, {}, 0.0f);
	}
}

void Mission::changeStatus(Status newStatus) {
	Status oldStatus = status;
	status = newStatus;
	if (newStatus != Mission::PENDING && newStatus != Mission::SELECTED) timeElapsed = 0.0f;

	if (oldStatus == Mission::SELECTED && newStatus == Mission::PENDING) {
		for (auto hero : assignedHeroes) hero->changeStatus(Hero::AVAILABLE, {}, 0.0f);
		assignedHeroes.clear();
	} else if (oldStatus == Mission::SELECTED && newStatus == Mission::TRAVELLING) for (auto hero : assignedHeroes) hero->changeStatus(Hero::TRAVELLING);
	else if (oldStatus == Mission::TRAVELLING && newStatus == Mission::PROGRESS) {}
	else if (oldStatus == Mission::PROGRESS && newStatus == Mission::AWAITING_REVIEW) for (auto hero : assignedHeroes) hero->changeStatus(Hero::RETURNING);
	else if (oldStatus == Mission::AWAITING_REVIEW) {
		std::cout << "Mission " << name << " completed, it was a " << (newStatus==Mission::REVIEWING_SUCESS ? "success" : "failure") << std::endl;
		if (newStatus == Mission::REVIEWING_SUCESS) {
			// TODO:
			// int exp = 1000 / std::sqrt(getSuccessChance() || 0.001f);
			// for (auto hero : assignedHeroes) hero.addExp(exp);
		} else if (newStatus == Mission::REVIEWING_FAILURE && dangerous) {
			auto hero = Utils::random_element(assignedHeroes);
			hero->wound();
			std::cout << hero->name << " was wounded" << std::endl;
		}
	} else if (newStatus == Mission::DONE || newStatus == Mission::MISSED) {
		for (auto& hero : assignedHeroes) {
			if (hero->status == Hero::AWAITING_REVIEW) hero->changeStatus(Hero::AVAILABLE, {}, 0.0f);
			else hero->mission.reset();
		}
	} else throw std::invalid_argument("Invalid mission status change");
}

void Mission::update(float deltaTime) {
	int working = std::count_if(assignedHeroes.begin(), assignedHeroes.end(), [&](auto hero){ return hero->status == Hero::WORKING; });
	switch (status) {
		case Mission::PENDING:
			timeElapsed += deltaTime;
			if (timeElapsed >= failureTime) changeStatus(Mission::MISSED);
			break;
		case Mission::TRAVELLING:
			if (working > 0) changeStatus(Mission::PROGRESS);
			break;
		case Mission::PROGRESS:
			timeElapsed += deltaTime * working / assignedHeroes.size();
			if (timeElapsed >= missionDuration) changeStatus(Mission::AWAITING_REVIEW);
			break;
		case Mission::SELECTED:
		case Mission::AWAITING_REVIEW:
		case Mission::REVIEWING_SUCESS:
		case Mission::REVIEWING_FAILURE:
			break;
		default:
			timeElapsed += deltaTime;
			break;
	}
}

void Mission::renderUI(bool full) {
	static raylib::Font defaultFont{};
	static raylib::Font emojiFont{"resources/fonts/NotoEmoji-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};
	static raylib::Font symbolsFont{"resources/fonts/NotoSansSymbols2-Regular.ttf", 32, (int[]){ 0x2713, 0x2714, 0x1F3C3, 0x1F5F8, 0 }, 4};
	static raylib::Font fontTitle{"resources/fonts/NotoSans-Bold.ttf", 32};
	static raylib::Font fontText{"resources/fonts/NotoSans-Regular.ttf", 22};

	raylib::Color bgLgt{244, 225, 203}, bgMed{198, 175, 145}, bgDrk{114, 100, 86}, textColor = ColorLerp(BROWN, BLACK, 0.8f), shadow = ColorAlpha(BLACK, 0.4);

	float progress = 0.0f;
	if (full) {
		// MAIN BACKGROUND PANEL
		raylib::Rectangle mainRect{(GetScreenWidth() - 800.0f) / 2, 10, 800, 400};

		// LEFT PANEL (incident source)
		raylib::Rectangle leftRect{mainRect.x + 10, mainRect.y + 50, 220, 320};
		raylib::Rectangle{leftRect.GetPosition() + raylib::Vector2{5,5}, leftRect.GetSize()}.Draw(shadow);
		leftRect.DrawGradient(bgMed, bgMed, bgDrk, bgMed);
		// Type
		raylib::Vector2 typePos = leftRect.GetPosition() + raylib::Vector2{10, 10};
		fontText.DrawText("CRIME TYPE", typePos, 18, 1, textColor);
		raylib::Vector2 linePos = {typePos.x + fontText.MeasureText("CRIME TYPE", 18, 1).x + 4, leftRect.y + 10};
		for (int i = 1; i <= 5; i++) {
			linePos.DrawLine({leftRect.x + leftRect.width - 4, linePos.y}, (i*i)%3 + 1, bgDrk);
			linePos.y += 5;
		}
		// Image
		raylib::Rectangle imgRect{leftRect.x + 5, typePos.y + 30, leftRect.width - 10, 100};
		imgRect.Draw(bgDrk);
		imgRect.DrawLines(BLACK);
		// Caller
		raylib::Rectangle callerBox{leftRect.x + 5, imgRect.y + imgRect.height, leftRect.width - 10, 40};
		callerBox.DrawLines(BLACK);
		Utils::drawTextCenteredY("Caller: Ronaldo", raylib::Vector2{callerBox.x + 5, callerBox.y + callerBox.height/2}, fontText, 18, textColor);
		// Description
		raylib::Rectangle descriptionBox{leftRect.x + 5, callerBox.y + callerBox.height, leftRect.width - 10, leftRect.y + leftRect.height - (callerBox.y + callerBox.height) - 5};
		descriptionBox.DrawGradient(bgLgt, bgMed, bgLgt, bgMed);
		descriptionBox.DrawLines(BLACK);
		Utils::drawTextCentered("\"Someone's robbing\n the museum...\"", Utils::center(descriptionBox), fontText, 14, textColor);

		// CENTER PANEL (title + radar + heroes + buttons)
		raylib::Rectangle centerRect{leftRect.x + leftRect.width + 10, leftRect.y-44, 340, 380};
		raylib::Rectangle{centerRect.GetPosition() + raylib::Vector2{5,5}, centerRect.GetSize()}.Draw(shadow);
		centerRect.Draw(bgMed);
		centerRect = Utils::inset(centerRect, 4);
		// Title Bar
		raylib::Rectangle titleRect{centerRect.x, centerRect.y, centerRect.width, 40.0f};
		titleRect.DrawGradient(ORANGE, ORANGE, ORANGE, ColorLerp(ORANGE, YELLOW, 0.7));
		titleRect.DrawLines(BLACK);
		Utils::drawTextCentered(name, Utils::center(titleRect), fontTitle, 24);
		// BUTTONS
		raylib::Rectangle btnsRect{centerRect.x, centerRect.y + centerRect.height - 30, centerRect.width, 28};
		std::vector<std::pair<std::string, raylib::Rectangle&>> btns{{"CLOSE", btnCancel}};
		if (status == Mission::SELECTED) btns.emplace_back("DISPATCH", btnStart);
		for (auto [idx, btn] : Utils::enumerate(btns)) {
			auto [name, rect] = btn; auto sz = btns.size(); auto width = btnsRect.width/sz;
			rect = raylib::Rectangle{btnsRect.x + idx*(width+2), btnsRect.y, width - 2, btnsRect.height};
			rect.Draw(bgLgt);
			rect.DrawLines(BLACK);
			Utils::drawTextCentered(name, Utils::center(rect), fontText, 18, BLACK);
		}
		if (status == Mission::SELECTED && assignedHeroes.empty()) btnStart.Draw(Fade(bgDrk, 0.6f));
		// Main panel (radar graph + heroes)
		raylib::Rectangle mainPanel{centerRect.x, titleRect.y + titleRect.height, centerRect.width, btnsRect.y - (titleRect.y + titleRect.height) - 4};
		mainPanel.DrawLines(BLACK);
		Utils::inset(mainPanel, 2).DrawGradient(bgLgt, bgLgt, bgMed, bgLgt);
		// Radar graph
		raylib::Vector2 radarCenter{mainPanel.x + mainPanel.width/2, mainPanel.y + 5*mainPanel.height/14};
		auto totalAttributes = getTotalAttributes();
		std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attrs{{totalAttributes, ORANGE, true}};
		if (status != Mission::SELECTED) {
			attrs.emplace_back(requiredAttributes, LIGHTGRAY, true);
			AttrMap<int> intersect; for (auto& [k, v] : requiredAttributes) intersect[k] = std::min(v, totalAttributes[k]);
			attrs.emplace_back(intersect, status == Mission::REVIEWING_SUCESS ? GREEN : RED, false);
		}
		Utils::drawRadarGraph(radarCenter, 60.0f, attrs, textColor, BROWN);
		// Hero portraits
		float start = mainPanel.x + (mainPanel.width - (slots*74 - 10)) / 2;
		raylib::Rectangle heroRect{start, mainPanel.y + mainPanel.height - 74, 64, 64};
		for (const auto& hero : assignedHeroes) {
			heroRect.Draw(Fade(DARKGRAY, 0.5f));
			heroRect.DrawLines(GRAY);
			Utils::drawTextCentered(hero->name.substr(0, 9), Utils::center(heroRect), fontText, 14, textColor);
			// TODO: Draw hero portrait
			heroRect.x += 74;
		}
		for (size_t i = assignedHeroes.size(); i < static_cast<size_t>(slots); i++) {
			heroRect.Draw(Fade(LIGHTGRAY, 0.4f));
			heroRect.DrawLines(GRAY);
			Utils::drawTextCentered("Empty Slot", Utils::center(heroRect), fontText, 14, textColor);
			heroRect.x += 74;
		}
		centerRect = Utils::inset(centerRect, -4);

		// RIGHT PANEL (requirements)
		raylib::Rectangle rightRect{centerRect.x + centerRect.width + 10, leftRect.y, 210, 320};
		raylib::Rectangle{rightRect.GetPosition() + raylib::Vector2{5,5}, rightRect.GetSize()}.Draw(shadow);
		rightRect.Draw(bgMed);
		raylib::Vector2 requirementsPos = rightRect.GetPosition() + raylib::Vector2{10, 5};
		fontText.DrawText("Requirements", requirementsPos, 18, 1, textColor);
		linePos = raylib::Vector2{requirementsPos.x + fontText.MeasureText("Requirements", 18, 1).x + 4, rightRect.y + 10};
		for (int i = 1; i <= 5; i++) {
			linePos.DrawLine({rightRect.x + rightRect.width - 4, linePos.y}, (i*i*i)%3 + 1, bgDrk);
			linePos.y += 5;
		}

		raylib::Rectangle reqsRet = Utils::inset(rightRect, {4, 30}); reqsRet.y += 10;
		reqsRet.DrawLines(BLACK);
		Utils::inset(reqsRet, 2).DrawGradient(bgLgt, bgLgt, bgMed, bgLgt);
		std::vector<std::pair<std::string, float>> reqs = {
			{"Security system taken over by robbers", 0},
			{"Avoid motion sensors", 0},
			{"Apprehend the thieves", 0}
		};
		float indentSize = fontText.MeasureText("> ", 16, 1).x, totalH=0;
		for (auto& [txt, h] : reqs) {
			txt = Utils::addLineBreaks(txt, reqsRet.width - 8 - indentSize, fontText, 16, 1);
			h = fontText.MeasureText(txt, 16, 1).y;
			if (totalH) totalH += 10;
			totalH += h;
		}
		raylib::Vector2 reqPos = {reqsRet.x + 4 + indentSize, reqsRet.y + (reqsRet.height - totalH) / 2 + 4 };
		for (auto& [txt, h] : reqs) {
			fontText.DrawText("> ", {reqPos.x - indentSize, reqPos.y}, 16, 1, textColor);
			fontText.DrawText(txt, reqPos, 16, 1, textColor);
			reqPos.y += h + 10;
		}
	} else {
		std::string text = "!";
		raylib::Font* font = &defaultFont;
		raylib::Color textColor{RED}, backgroundColor{ORANGE}, timeRemainingColor{LIGHTGRAY}, timeElapsedColor{GRAY};
		switch (status) {
			case Mission::PENDING:
				progress = timeElapsed / failureTime;
				break;
			case Mission::SELECTED:
				progress = timeElapsed / failureTime;
				textColor = WHITE;
				backgroundColor = SKYBLUE;
				timeElapsedColor = BLUE;
				timeRemainingColor = WHITE;
				break;
			case Mission::PROGRESS:
				progress = timeElapsed / missionDuration;
			case Mission::TRAVELLING:
				text = "🏃";
				font = &emojiFont;
				textColor = WHITE;
				backgroundColor = SKYBLUE;
				timeElapsedColor = BLUE;
				timeRemainingColor = WHITE;
				break;
			case Mission::AWAITING_REVIEW:
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
		if (status == Mission::PENDING) {
			if (mousePos.CheckCollision(position, 28)) status = Mission::SELECTED;
		} else if (status == Mission::SELECTED) {
			if (mousePos.CheckCollision(btnCancel)) changeStatus(Mission::PENDING);
			else if (mousePos.CheckCollision(btnStart) && !assignedHeroes.empty()) changeStatus(Mission::TRAVELLING);
		} else if (status == Mission::AWAITING_REVIEW) {
			if (mousePos.CheckCollision(position, 28)) status = isSuccessful() ? Mission::REVIEWING_SUCESS : Mission::REVIEWING_FAILURE;
		} else if (status == Mission::REVIEWING_SUCESS || status == Mission::REVIEWING_FAILURE) {
			if (mousePos.CheckCollision(btnCancel)) changeStatus(Mission::DONE);
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
