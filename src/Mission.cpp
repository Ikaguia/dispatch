#include <cctype>
#include <algorithm>
#include <iostream>
#include <format>
#include <memory>
#include <fstream>

#include <Utils.hpp>
#include <Common.hpp>
#include <Mission.hpp>
#include <Hero.hpp>
#include <MissionsHandler.hpp>

Mission::Mission(
	const std::string& name,
	const std::string& type,
	const std::string& caller,
	const std::string& description,
	const std::vector<std::string>& requirements,
	raylib::Vector2 position,
	const std::map<std::string, int> &attr,
	int slots,
	int difficulty,
	float failureTime,
	float missionDuration,
	bool dangerous
) :
	name{name},
	type{type},
	caller{caller},
	description{description},
	requirements{requirements},
	position{position},
	requiredAttributes{},
	slots{slots},
	difficulty{difficulty},
	failureTime{failureTime},
	missionDuration{missionDuration},
	dangerous{dangerous}
{
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		requiredAttributes[attribute] = value;
	}
};

Mission::Mission(const std::string& fileName) { load(fileName); }
Mission::Mission(std::ifstream& input) { load(input); }

void Mission::load(const std::string& fileName) {
	std::ifstream input{fileName};
	load(input);
}
void Mission::load(std::ifstream& input) {
	int n;
	std::getline(input, name);
	std::getline(input, type);
	std::getline(input, caller);
	std::getline(input, description);
	input >> n; input.ignore();
	requirements.resize(n);
	for (int i = 0; i < n; i++) std::getline(input, requirements[i]);
	input >> position.x >> position.y;
	for (Attribute attr : Attribute::Values) input >> requiredAttributes[attr];
	input >> slots;
	input >> difficulty;
	input >> failureTime;
	input >> missionDuration;
	input >> dangerous; input.ignore();
	Utils::println("Loaded mission {}", name);
	Utils::println("  Type: {}, Caller: {}", type, caller);
	Utils::println("  Description: {}", description);
	Utils::println("  Position: ({}, {}), Slots: {}, Difficulty: {}, Dangerous: {}", position.x, position.y, slots, difficulty, dangerous);
	for (int i = 0; i < n; i++) Utils::println("  Requirement {}: {}", i+1, requirements[i]);
	for (Attribute attr : Attribute::Values) Utils::println("  Required {}: {}", attr.toString(), requiredAttributes[attr]);
	validate();
}
void Mission::validate() const {
	if (name.empty()) throw std::invalid_argument("Mission name cannot be empty");
	if (type.empty()) throw std::invalid_argument("Mission type cannot be empty");
	if (caller.empty()) throw std::invalid_argument("Mission caller cannot be empty");
	if (description.empty()) throw std::invalid_argument("Mission description cannot be empty");
	if (slots <= 0 || slots > 4) throw std::invalid_argument("Mission slots must be between 1 and 4");
	if (difficulty < 1 || difficulty > 5) throw std::invalid_argument("Mission difficulty must be between 1 and 5");
	if (failureTime < 1) throw std::invalid_argument("Mission failure time must be positive");
	if (failureTime > 1000) throw std::invalid_argument("Mission failure time must be less than 1000 seconds");
	if (missionDuration < 1) throw std::invalid_argument("Mission duration must be positive");
	if (missionDuration > 1000) throw std::invalid_argument("Mission duration must be less than 1000 seconds");
	if (requirements.size() < 1 || requirements.size() > 5) throw std::invalid_argument("Mission must have between 1 and 5 requirements");
	for (Attribute attr : Attribute::Values) if (requiredAttributes[attr] < 0 || requiredAttributes[attr] > 10) throw std::invalid_argument(std::format("Mission required attribute {} must be between 0 and 10", attr.toString()));
	if (position.x < 50 || position.x > 900 || position.y < 50 || position.y > 350) throw std::invalid_argument("Mission position is out of bounds");
}


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

	if (oldStatus == Mission::PENDING && newStatus == Mission::SELECTED) {}
	else if (oldStatus == Mission::SELECTED && newStatus == Mission::PENDING) {
		for (auto hero : assignedHeroes) hero->changeStatus(Hero::AVAILABLE, {}, 0.0f);
		assignedHeroes.clear();
	} else if (oldStatus == Mission::SELECTED && newStatus == Mission::TRAVELLING) for (auto hero : assignedHeroes) hero->changeStatus(Hero::TRAVELLING);
	else if (oldStatus == Mission::TRAVELLING && newStatus == Mission::PROGRESS) {}
	else if (oldStatus == Mission::PROGRESS && newStatus == Mission::AWAITING_REVIEW) for (auto hero : assignedHeroes) hero->changeStatus(Hero::RETURNING);
	else if (oldStatus == Mission::AWAITING_REVIEW) {
		std::cout << "Mission " << name << " completed, it was a " << (newStatus==Mission::REVIEWING_SUCESS ? "success" : "failure") << std::endl;
		if (newStatus == Mission::REVIEWING_SUCESS) {
			int exp = 200 * difficulty / std::sqrt(getSuccessChance() || 0.001f) / assignedHeroes.size();
			for (auto& hero : assignedHeroes) hero->addExp(exp);
			Utils::println("Each hero gained {} exp", exp);
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
	float progress = 0.0f;
	if (full) {
		// MAIN BACKGROUND PANEL
		raylib::Rectangle mainRect{(GetScreenWidth() - 800.0f) / 2, 10, 800, 400};

		// LEFT PANEL (incident source)
		raylib::Rectangle leftRect{mainRect.x + 10, mainRect.y + 50, 220, 320};
		raylib::Rectangle{leftRect.GetPosition() + raylib::Vector2{5,5}, leftRect.GetSize()}.Draw(Dispatch::UI::shadow);
		leftRect.DrawGradient(Dispatch::UI::bgMed, Dispatch::UI::bgMed, Dispatch::UI::bgDrk, Dispatch::UI::bgMed);
		// Type
		raylib::Vector2 typePos = leftRect.GetPosition() + raylib::Vector2{10, 10};
		Dispatch::UI::fontText.DrawText(type, typePos, 18, 1, Dispatch::UI::textColor);
		raylib::Vector2 linePos = {typePos.x + Dispatch::UI::fontText.MeasureText(type, 18, 1).x + 4, leftRect.y + 10};
		for (int i = 1; i <= 5; i++) {
			linePos.DrawLine({leftRect.x + leftRect.width - 4, linePos.y}, (i*i)%3 + 1, Dispatch::UI::bgDrk);
			linePos.y += 5;
		}
		// Image
		raylib::Rectangle imgRect{leftRect.x + 5, typePos.y + 30, leftRect.width - 10, 100};
		imgRect.Draw(Dispatch::UI::bgDrk);
		imgRect.DrawLines(BLACK);
		// Caller
		raylib::Rectangle callerBox{leftRect.x + 5, imgRect.y + imgRect.height, leftRect.width - 10, 40};
		callerBox.DrawLines(BLACK);
		Utils::drawTextCenteredY(std::format("Caller: {}", caller), raylib::Vector2{callerBox.x + 5, callerBox.y + callerBox.height/2}, Dispatch::UI::fontText, 18, Dispatch::UI::textColor);
		// Description
		raylib::Rectangle descriptionBox{leftRect.x + 5, callerBox.y + callerBox.height, leftRect.width - 10, leftRect.y + leftRect.height - (callerBox.y + callerBox.height) - 5};
		descriptionBox.DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt, Dispatch::UI::bgMed);
		descriptionBox.DrawLines(BLACK);
		std::string desc = Utils::addLineBreaks(description, descriptionBox.width, Dispatch::UI::fontText, 14, 2);
		Utils::drawTextCentered(desc, Utils::center(descriptionBox), Dispatch::UI::fontText, 14, Dispatch::UI::textColor);

		// CENTER PANEL (title + radar + heroes + buttons)
		raylib::Rectangle centerRect{leftRect.x + leftRect.width + 10, leftRect.y-44, 340, 380};
		raylib::Rectangle{centerRect.GetPosition() + raylib::Vector2{5,5}, centerRect.GetSize()}.Draw(Dispatch::UI::shadow);
		centerRect.Draw(Dispatch::UI::bgMed);
		centerRect = Utils::inset(centerRect, 4);
		// Title Bar
		raylib::Rectangle titleRect{centerRect.x, centerRect.y, centerRect.width, 40.0f};
		titleRect.DrawGradient(ORANGE, ORANGE, ORANGE, ColorLerp(ORANGE, YELLOW, 0.7));
		titleRect.DrawLines(BLACK);
		Utils::drawTextCentered(name, Utils::center(titleRect), Dispatch::UI::fontTitle, 24);
		// BUTTONS
		raylib::Rectangle btnsRect{centerRect.x, centerRect.y + centerRect.height - 30, centerRect.width, 28};
		std::vector<std::pair<std::string, raylib::Rectangle&>> btns{{"CLOSE", btnCancel}};
		if (status == Mission::SELECTED) btns.emplace_back("DISPATCH", btnStart);
		for (auto [idx, btn] : Utils::enumerate(btns)) {
			auto [name, rect] = btn; auto sz = btns.size(); auto width = btnsRect.width/sz;
			rect = raylib::Rectangle{btnsRect.x + idx*(width+2), btnsRect.y, width - 2, btnsRect.height};
			rect.Draw(Dispatch::UI::bgLgt);
			rect.DrawLines(BLACK);
			Utils::drawTextCentered(name, Utils::center(rect), Dispatch::UI::fontText, 18, BLACK);
		}
		if (status == Mission::SELECTED && assignedHeroes.empty()) btnStart.Draw(Fade(Dispatch::UI::bgDrk, 0.6f));
		// Main panel (radar graph + heroes)
		raylib::Rectangle mainPanel{centerRect.x, titleRect.y + titleRect.height, centerRect.width, btnsRect.y - (titleRect.y + titleRect.height) - 4};
		mainPanel.DrawLines(BLACK);
		Utils::inset(mainPanel, 2).DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt);
		// Radar graph
		raylib::Vector2 radarCenter{mainPanel.x + mainPanel.width/2, mainPanel.y + ((status==Mission::SELECTED)?(5*mainPanel.height/14):(mainPanel.height/2))};
		auto totalAttributes = getTotalAttributes();
		std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attrs{{totalAttributes, ORANGE, true}};
		if (status != Mission::SELECTED) {
			attrs.emplace_back(requiredAttributes, LIGHTGRAY, true);
			AttrMap<int> intersect; for (auto& [k, v] : requiredAttributes) intersect[k] = std::min(v, totalAttributes[k]);
			attrs.emplace_back(intersect, status == Mission::REVIEWING_SUCESS ? GREEN : RED, false);
		}
		Utils::drawRadarGraph(radarCenter, (status==Mission::SELECTED?60.0f:90.0f), attrs, Dispatch::UI::textColor, BROWN);
		if (status == Mission::SELECTED) {
			// Hero portraits
			float start = mainPanel.x + (mainPanel.width - (slots*74 - 10)) / 2;
			raylib::Rectangle heroRect{start, mainPanel.y + mainPanel.height - 74, 64, 64};
			for (const auto& hero : assignedHeroes) {
				heroRect.Draw(Fade(DARKGRAY, 0.5f));
				heroRect.DrawLines(GRAY);
				Utils::drawTextCentered(hero->name.substr(0, 9), Utils::center(heroRect), Dispatch::UI::fontText, 14, Dispatch::UI::textColor);
				// TODO: Draw hero portrait
				heroRect.x += 74;
			}
			for (size_t i = assignedHeroes.size(); i < static_cast<size_t>(slots); i++) {
				heroRect.Draw(Fade(LIGHTGRAY, 0.4f));
				heroRect.DrawLines(GRAY);
				Utils::drawTextCentered("Empty Slot", Utils::center(heroRect), Dispatch::UI::fontText, 14, Dispatch::UI::textColor);
				heroRect.x += 74;
			}
		} else {
			raylib::Rectangle reviewRect{mainPanel.x + 6, mainPanel.y + mainPanel.height - 35, mainPanel.width - 12, 32};
			raylib::Rectangle chanceRect{mainPanel.x + mainPanel.width/2 - 45, mainPanel.y + mainPanel.height - 72, 90, 34};
			reviewRect.Draw(Dispatch::UI::bgDrk);
			chanceRect.Draw(GOLD);
			Utils::inset(reviewRect, 1).DrawLines(WHITE);
			Utils::inset(chanceRect, 1).DrawLines(WHITE);
			reviewRect.DrawLines(Dispatch::UI::bgDrk);
			chanceRect.DrawLines(Dispatch::UI::bgDrk);
			std::string reviewText = std::format("MISSION {}!", status == Mission::REVIEWING_SUCESS ? "COMPLETED" : "FAILED");
			std::string chanceText = std::format("{}%", getSuccessChance());
			std::vector<std::tuple<std::string, raylib::Font&, int, raylib::Color, int, raylib::Color, float>> chanceTexts = {
				{std::string{"🎯"}, Dispatch::UI::emojiFont, 32, WHITE, 2, raylib::Color{0,0,0,0}, 0.0f},
				{chanceText, Dispatch::UI::defaultFont, 20, WHITE, 2, raylib::Color{0,0,0,0}, 0.0f}
			};
			Utils::drawTextCentered(reviewText, Utils::center(reviewRect), Dispatch::UI::fontText, 20, WHITE);
			Utils::drawTextSequence(chanceTexts, Utils::center(chanceRect), true, true, 2, true);
		}
		centerRect = Utils::inset(centerRect, -4);

		// RIGHT PANEL (requirements)
		raylib::Rectangle rightRect{centerRect.x + centerRect.width + 10, leftRect.y, 210, 320};
		raylib::Rectangle{rightRect.GetPosition() + raylib::Vector2{5,5}, rightRect.GetSize()}.Draw(Dispatch::UI::shadow);
		rightRect.Draw(Dispatch::UI::bgMed);
		raylib::Vector2 requirementsPos = rightRect.GetPosition() + raylib::Vector2{10, 5};
		Dispatch::UI::fontText.DrawText("Requirements", requirementsPos, 18, 1, Dispatch::UI::textColor);
		linePos = raylib::Vector2{requirementsPos.x + Dispatch::UI::fontText.MeasureText("Requirements", 18, 1).x + 4, rightRect.y + 10};
		for (int i = 1; i <= 5; i++) {
			linePos.DrawLine({rightRect.x + rightRect.width - 4, linePos.y}, (i*i*i)%3 + 1, Dispatch::UI::bgDrk);
			linePos.y += 5;
		}

		raylib::Rectangle reqsRet = Utils::inset(rightRect, {4, 30}); reqsRet.y += 10;
		reqsRet.DrawLines(BLACK);
		Utils::inset(reqsRet, 2).DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt);
		std::vector<std::pair<std::string, float>> reqs;
		for (const auto& req : requirements) reqs.emplace_back(req, 0.0f);
		float indentSize = Dispatch::UI::fontText.MeasureText("> ", 16, 1).x, totalH=0;
		for (auto& [txt, h] : reqs) {
			txt = Utils::addLineBreaks(txt, reqsRet.width - 8 - indentSize, Dispatch::UI::fontText, 16, 1);
			h = Dispatch::UI::fontText.MeasureText(txt, 16, 1).y;
			if (totalH) totalH += 10;
			totalH += h;
		}
		raylib::Vector2 reqPos = {reqsRet.x + 4 + indentSize, reqsRet.y + (reqsRet.height - totalH) / 2 + 4 };
		for (auto& [txt, h] : reqs) {
			Dispatch::UI::fontText.DrawText("> ", {reqPos.x - indentSize, reqPos.y}, 16, 1, Dispatch::UI::textColor);
			Dispatch::UI::fontText.DrawText(txt, reqPos, 16, 1, Dispatch::UI::textColor);
			reqPos.y += h + 10;
		}
	} else {
		std::string text = "!";
		raylib::Font* font = &Dispatch::UI::defaultFont;
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
				font = &Dispatch::UI::emojiFont;
				textColor = WHITE;
				backgroundColor = SKYBLUE;
				timeElapsedColor = BLUE;
				timeRemainingColor = WHITE;
				break;
			case Mission::AWAITING_REVIEW:
				text = "✔";
				font = &Dispatch::UI::symbolsFont;
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
			if (mousePos.CheckCollision(position, 28)) changeStatus(Mission::SELECTED);
		} else if (status == Mission::SELECTED) {
			if (mousePos.CheckCollision(btnCancel)) changeStatus(Mission::PENDING);
			else if (mousePos.CheckCollision(btnStart) && !assignedHeroes.empty()) changeStatus(Mission::TRAVELLING);
		} else if (status == Mission::AWAITING_REVIEW) {
			if (mousePos.CheckCollision(position, 28)) changeStatus(isSuccessful() ? Mission::REVIEWING_SUCESS : Mission::REVIEWING_FAILURE);
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
