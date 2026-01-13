#include <cctype>
#include <algorithm>
#include <iostream>
#include <format>
#include <memory>
#include <fstream>

#include <nlohmann/json.hpp>
using nlohmann::json;

#include <Utils.hpp>
#include <Common.hpp>
#include <Mission.hpp>
#include <Hero.hpp>
#include <MissionsHandler.hpp>
#include <HeroesHandler.hpp>
#include <EventHandler.hpp>

Mission::Mission(
	const std::string& new_name,
	const std::string& new_type,
	const std::string& new_caller,
	const std::string& new_description,
	const std::string& new_failureMsg,
	const std::string& new_failureMission,
	const std::string& new_successMsg,
	const std::string& new_successMission,
	const std::vector<std::string>& new_requirements,
	raylib::Vector2 new_position,
	const std::unordered_map<std::string, int> &new_requiredAttributes,
	int new_slots,
	int new_difficulty,
	float new_failureTime,
	float new_missionDuration,
	float new_failureMissionTime,
	float new_successMissionTime,
	bool new_dangerous
) :
	name{new_name},
	type{new_type},
	caller{new_caller},
	description{new_description},
	failureMsg{new_failureMsg},
	failureMission{new_failureMission},
	successMsg{new_successMsg},
	successMission{new_successMission},
	requirements{new_requirements},
	position{new_position},
	requiredAttributes{},
	slots{new_slots},
	difficulty{new_difficulty},
	failureTime{new_failureTime},
	missionDuration{new_missionDuration},
	failureMissionTime{new_failureMissionTime},
	successMissionTime{new_successMissionTime},
	dangerous{new_dangerous}
{
	for (auto [attrName, value] : new_requiredAttributes) {
		Attribute attribute = Attribute::fromString(attrName);
		requiredAttributes[attribute] = value;
	}
	assignedSlots.resize(slots);
};

Mission::Mission(const json& data) { from_json(data, *this); validate(); }

void Mission::validate() const {
	if (name.empty()) throw std::invalid_argument("Mission name cannot be empty");
	if (type.empty()) throw std::invalid_argument("Mission type cannot be empty");
	if (caller.empty()) throw std::invalid_argument("Mission caller cannot be empty");
	if (description.empty()) throw std::invalid_argument("Mission description cannot be empty");
	if (failureMsg.empty()) throw std::invalid_argument("Mission failure message cannot be empty");
	if (successMsg.empty()) throw std::invalid_argument("Mission success message cannot be empty");
	if (failureMission.empty() && failureMissionTime!=0.0f) throw std::invalid_argument("Mission 'failureMissionTime' must be 0 when no failure mission is set");
	if (!failureMission.empty() && failureMissionTime<=0.0f) throw std::invalid_argument("Mission 'failureMissionTime' must be > 0 when failure mission is set");
	if (successMission.empty() && successMissionTime!=0.0f) throw std::invalid_argument("Mission 'successMissionTime' must be 0 when no success mission is set");
	if (!successMission.empty() && successMissionTime<=0.0f) throw std::invalid_argument("Mission 'successMissionTime' must be > 0 when success mission is set");
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


void Mission::toggleHero(const std::string& hero_name) {
	bool assigned = assignedHeroes.contains(hero_name);
	if (assigned) unassignHero(hero_name);
	else assignHero(hero_name);
}

void Mission::assignHero(const std::string& hero_name) {
	auto& hero = HeroesHandler::inst()[hero_name];
	int cnt = (int)(assignedHeroes.size());
	if (cnt < slots && hero.status == Hero::AVAILABLE) {
		assignedHeroes.insert(hero_name);
		for (int i = 0; i < slots; i++) {
			if (assignedSlots[i].empty()) {
				assignedSlots[i] = hero_name;
				break;
			}
		}
		hero.changeStatus(Hero::ASSIGNED, name);
	}
	auto& layout = MissionsHandler::inst().layoutMissionDetails;
	updateLayout(layout, "assignedHeroes");
}

void Mission::unassignHero(const std::string& hero_name) {
	auto& hero = HeroesHandler::inst()[hero_name];
	if (assignedHeroes.contains(hero_name)) {
		assignedHeroes.erase(hero_name);
		for (int i = 0; i < slots; i++) {
			if (assignedSlots[i] == hero_name) {
				assignedSlots[i].clear();
				break;
			}
		}
		hero.changeStatus(Hero::AVAILABLE, {}, 0.0f);
	}
	auto& layout = MissionsHandler::inst().layoutMissionDetails;
	updateLayout(layout, "assignedHeroes");
}

void Mission::changeStatus(Status newStatus) {
	auto& eh = EventHandler::inst();
	Status oldStatus = status;
	status = newStatus;
	if (newStatus != Mission::PENDING && newStatus != Mission::SELECTED && newStatus != Mission::DISRUPTION && oldStatus != Mission::DISRUPTION) timeElapsed = 0.0f;

	if (oldStatus == Mission::PENDING && newStatus == Mission::SELECTED) {}
	else if (oldStatus == Mission::SELECTED && newStatus == Mission::PENDING) {
		for (auto& hero_name : assignedHeroes) HeroesHandler::inst()[hero_name].changeStatus(Hero::AVAILABLE, {}, 0.0f);
		assignedHeroes.clear();
		for (auto& slot : assignedSlots) if (!slot.empty()) slot.clear();
	} else if (oldStatus == Mission::SELECTED && newStatus == Mission::TRAVELLING) {
		EventData ed = MissionStartData{name, &assignedSlots};
		eh.emit<Event::MissionStart>(assignedHeroes, name, &assignedSlots);
		for (auto hero_name : assignedHeroes) HeroesHandler::inst()[hero_name].changeStatus(Hero::TRAVELLING);
	} else if (oldStatus == Mission::TRAVELLING && newStatus == Mission::PROGRESS) {
	} else if (oldStatus == Mission::PROGRESS && newStatus == Mission::DISRUPTION) {
		curDisruption++;
		disruptions[curDisruption].elapsedTime = 0.0f;
	} else if (oldStatus == Mission::DISRUPTION && newStatus == Mission::DISRUPTION_MENU) {
		auto& disruption = disruptions[curDisruption];
		for (auto& option : disruption.options) {
			if (option.type == Disruption::Option::HERO) {
				option.disabled = !std::any_of(BEGEND(assignedHeroes), [&](auto& hero_name){ return hero_name == option.hero; });
			} else option.disabled = false;
		}
	} else if (oldStatus == Mission::DISRUPTION && newStatus == Mission::PROGRESS) {
		disrupted = true;
		curDisruption = disruptions.size();
	} else if (oldStatus == Mission::PROGRESS && newStatus == Mission::AWAITING_REVIEW) for (auto& hero_name : assignedHeroes) HeroesHandler::inst()[hero_name].changeStatus(Hero::RETURNING);
	else if (oldStatus == Mission::AWAITING_REVIEW) {
		bool success = newStatus == Mission::REVIEWING_SUCESS;
		if (success) eh.emit<Event::MissionSuccess>(assignedHeroes, name, &assignedSlots);
		else eh.emit<Event::MissionFailure>(assignedHeroes, name, &assignedSlots);
		Utils::println("Mission {} completed, it was a {}", name, newStatus==Mission::REVIEWING_SUCESS ? "success" : "failure");
	} else if (newStatus == Mission::DONE || newStatus == Mission::MISSED) {
		if (oldStatus == Mission::REVIEWING_SUCESS && !disrupted) {
			float successChance = getSuccessChance();
			float chanceMult = 10.0f / std::sqrt(successChance ? successChance : 1);
			float difficultyMult = std::sqrt(difficulty);
			float heroCountDiv = std::sqrt(assignedHeroes.size());
			int exp = 500 * chanceMult * difficultyMult / heroCountDiv;
			for (auto& hero_name : assignedHeroes) HeroesHandler::inst()[hero_name].addExp(exp);
			Utils::println("Each hero gained {} exp", exp);
			if (!successMission.empty()) MissionsHandler::inst().addMissionToQueue(successMission, successMissionTime);
		} else if ((oldStatus == Mission::REVIEWING_FAILURE || disrupted) && dangerous) {
			auto& hero_name = Utils::random_element(assignedHeroes);
			auto& hero = HeroesHandler::inst()[hero_name];
			hero.wound();
			Utils::println("{} was wounded", hero.name);
			if (!failureMission.empty()) MissionsHandler::inst().addMissionToQueue(failureMission, failureMissionTime);
		}
		for (auto& hero_name : assignedHeroes) {
			auto& hero = HeroesHandler::inst()[hero_name];
			if (hero.status == Hero::AWAITING_REVIEW) hero.changeStatus(Hero::AVAILABLE, {}, 0.0f);
			else if (hero.status == Hero::WORKING) hero.changeStatus(Hero::RETURNING, {}, 0.0f);
			else hero.mission.clear();
		}
	} else {
		Utils::println("Invalid mission status change, from {} to {}", statusToString(oldStatus), statusToString(newStatus));
		throw std::invalid_argument(std::format("Invalid mission status change, from {} to {}", statusToString(oldStatus), statusToString(newStatus)));
	}

	auto& layout = MissionsHandler::inst().layoutMissionDetails;
	updateLayout(layout, "status");
}

void Mission::update(float deltaTime) {
	int working = std::count_if(BEGEND(assignedHeroes), [&](auto& hero_name){ return HeroesHandler::inst()[hero_name].status == Hero::WORKING; });
	switch (status) {
		case Mission::PENDING:
			timeElapsed += deltaTime;
			if (timeElapsed >= failureTime) changeStatus(Mission::MISSED);
			break;
		case Mission::TRAVELLING:
			if (working > 0) changeStatus(Mission::PROGRESS);
			break;
		case Mission::PROGRESS: {
			timeElapsed += deltaTime * working / assignedHeroes.size();
			int sz = static_cast<int>(disruptions.size());
			if (sz > 0 && curDisruption < sz) {
				float disruptionInterval = missionDuration / (1 + sz);
				float disruptionTime = (curDisruption + 2) * disruptionInterval;
				if (timeElapsed >= disruptionTime) changeStatus(Mission::DISRUPTION);
			}
			if (timeElapsed >= missionDuration) changeStatus(Mission::AWAITING_REVIEW);
			break;
		} case Mission::DISRUPTION: {
			auto& disruption = disruptions[curDisruption];
			disruption.elapsedTime += deltaTime;
			if (disruption.elapsedTime >= disruption.timeout) changeStatus(Mission::PROGRESS);
			break;
		} case Mission::SELECTED:
		case Mission::AWAITING_REVIEW:
		case Mission::REVIEWING_SUCESS:
		case Mission::REVIEWING_FAILURE:
		case Mission::DISRUPTION_MENU:
			break;
		default:
			timeElapsed += deltaTime;
			break;
	}
}

void Mission::renderUI() {
	float progress = 0.0f;
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
			// fallthrough
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
		case Mission::DISRUPTION:
		case Mission::DISRUPTION_MENU:
			text = "🛑";
			font = &Dispatch::UI::emojiFont;
			textColor = WHITE;
			backgroundColor = RED;
			timeRemainingColor = WHITE;
			timeElapsedColor = RED;
			progress = disruptions[curDisruption].elapsedTime / disruptions[curDisruption].timeout;
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
	Utils::drawTextCentered(text, position, *font, 36, WHITE);
}

void Mission::handleInput() {
	if (isMenuOpen()) {
		auto& layout = MissionsHandler::inst().layoutMissionDetails;

		if (status == Mission::SELECTED) {
			if (layout.clicked.contains("close")) {
				changeStatus(Mission::PENDING);
				layout.resetInput();
			} else if (layout.clicked.contains("dispatch") && !assignedHeroes.empty()) {
				changeStatus(Mission::TRAVELLING);
				layout.resetInput();
			} else {
				for (int i = 0; i < slots; i++) {
					auto& heroName = assignedSlots[i];
					if (!heroName.empty()) {
						std::string s_key = std::format("slots.children.{}-{}", i, heroName);
						if (layout.clicked.contains(s_key)) unassignHero(heroName);
					}
				}
			}
		} else if (status == Mission::REVIEWING_SUCESS || status == Mission::REVIEWING_FAILURE) {
			if (layout.clicked.contains("close-centered")) {
				changeStatus(Mission::DONE);
				layout.resetInput();
			}
		} else if (status == Mission::DISRUPTION_MENU) {
			auto& disruption = disruptions[curDisruption];
			if (disruption.selected_option == -1) {
				int sz = static_cast<int>(disruption.options.size());
				for (int idx = 0; idx < sz; idx++) {
					auto& option = disruption.options[idx];
					if (!option.disabled && layout.clicked.contains(std::format("disruption-options.children.{}", option.name))) {
						disruption.selected_option = idx;
						updateLayout(layout, "status");
					}
				}
			} else if (layout.clicked.contains("close-centered")) {
				disrupted |= !isDisruptionSuccessful();
				if (++curDisruption == (int)disruptions.size()) {
					changeStatus(Mission::DONE);
					layout.resetInput();
				}
			}
		}
	} else if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		if (status == Mission::PENDING) {
			if (mousePos.CheckCollision(position, 28)) changeStatus(Mission::SELECTED);
		} else if (status == Mission::AWAITING_REVIEW) {
			if (mousePos.CheckCollision(position, 28)) changeStatus(isSuccessful() ? Mission::REVIEWING_SUCESS : Mission::REVIEWING_FAILURE);
		} else if (status == Mission::DISRUPTION) {
			if (mousePos.CheckCollision(position, 28)) changeStatus(Mission::DISRUPTION_MENU);
		}
	}
}

void Mission::setupLayout(Dispatch::UI::Layout& layout) {
	AttrMap<int> overlap, totalAttributes = getTotalAttributes();
	for (Attribute::Value attr : Attribute::Values) overlap[attr] = std::min(requiredAttributes[attr], totalAttributes[attr]);
	layout.updateSharedData("type", type);
	layout.updateSharedData("caller", caller);
	layout.updateSharedData("description", description);
	layout.updateSharedData("name", name);
	layout.updateSharedData("requirements", Utils::join(requirements, "\n"));
	layout.updateSharedData("required-attributes", requiredAttributes);
	layout.updateSharedData("overlap-attributes", overlap);
	layout.updateSharedData("success-chance", std::format("{}%", getSuccessChance())); // TODO: Add icon "🎯"
	layout.updateSharedData("review-message", status == Status::REVIEWING_SUCESS ? successMsg : failureMsg);

	updateLayout(layout, "");
}
void Mission::updateLayout(Dispatch::UI::Layout& layout, const std::string& changed) {
	if (changed == "assignedHeroes" || changed == "") {
		std::vector<std::string> slotNames;
		for (int i = 0; i < slots; i++) {
			if (assignedSlots[i].empty()) {
				std::string s_key = std::format("{}-empty", i);
				slotNames.push_back(s_key);
				layout.deleteSharedData(s_key + "-image-key");
				layout.updateSharedData(s_key + "-disabled", true);
			} else {
				auto& hero = HeroesHandler::inst()[assignedSlots[i]];
				std::string s_key = std::format("{}-{}", i, hero.name);
				slotNames.push_back(s_key);
				layout.updateSharedData(s_key + "-image-key", std::format("hero-{}-portrait", hero.name));
				layout.updateSharedData(s_key + "-disabled", false);
			}
		}
		layout.updateSharedData("slot-names", slotNames);
		layout.updateSharedData("total-attributes", getTotalAttributes());

		auto* dispatch = layout.get<Dispatch::UI::Button>("dispatch");
		if (!dispatch) throw std::runtime_error("Mission details layout is missing 'dispatch' element or it is of the wrong type.");
		bool isDisabled = dispatch->status == Dispatch::UI::Element::Status::DISABLED;
		if (assignedHeroes.empty() && !isDisabled) dispatch->changeStatus(Dispatch::UI::Element::Status::DISABLED);
		if (!assignedHeroes.empty() && isDisabled) dispatch->changeStatus(Dispatch::UI::Element::Status::REGULAR);
	}
	if (changed == "status" || changed == "") {
		layout["main-selected"]->visible = status == Status::SELECTED;
		layout["main-reviewing"]->visible = (status == Status::REVIEWING_SUCESS) || (status == Status::REVIEWING_FAILURE);
		layout["dispatch"]->visible = status == Status::SELECTED;
		layout["close"]->visible = status == Status::SELECTED;
		layout["close-centered"]->visible = status != Status::SELECTED;

		layout["right-requirements"]->visible = status == Status::SELECTED || status == Status::REVIEWING_SUCESS || status == Status::REVIEWING_FAILURE;
		layout["right-assigned-stats"]->visible = status == Status::DISRUPTION_MENU;

		auto* attrGraph = layout.get<Dispatch::UI::AttrGraph>("result-attrs");
		attrGraph->overlap.color = status == Status::REVIEWING_SUCESS ? GREEN : RED;

		auto* dispatch = layout.get<Dispatch::UI::Button>("dispatch");
		dispatch->changeStatus(Dispatch::UI::Element::Status::DISABLED);

		if (status != Status::DISRUPTION_MENU) {
			layout["main-disruption"]->visible = false;
			layout["main-disruption-result"]->visible = false;
			layout["buttons"]->visible = true;
			layout["right-assigned-stats"]->visible = false;
			layout["right-choice-made"]->visible = false;
		} else {
			auto& disruption = disruptions[curDisruption];
			bool result = disruption.selected_option != -1;
			if (result) {
				auto selOpt = disruption.options[disruption.selected_option];
				bool success = isDisruptionSuccessful();
				bool isHero = selOpt.type == Disruption::Option::HERO;
				if (isHero) layout.updateSharedData("disruption-hero-image-key", std::format("hero-{}-portrait", selOpt.hero));
				else {
					AttrMap<int> required, total;
					for (Attribute::Value attr : Attribute::Values) {
						required[attr] = selOpt.value;
						total[attr] = getTotalAttribute(selOpt.attribute);
					}
					layout.updateSharedData("required-attributes", required);
					layout.updateSharedData("total-attributes", total);
					layout.updateSharedData("disruption-attribute", selOpt.attribute);
				}
				layout.updateSharedData("disruption-result-msg", success ? selOpt.successMessage : selOpt.failureMessage);

				for (auto [idx, opt] : Utils::enumerate(disruption.options)) {
					auto* btn = layout.get<Dispatch::UI::Button>(std::format("choice-made-options.children.{}", opt.name));
					btn->changeStatus((int)idx == disruption.selected_option ? btn->SELECTED : btn->DISABLED);
				}

				layout["disruption-result-hero"]->visible = isHero;
				layout["disruption-result-regular"]->visible = !isHero;
				layout.get<Dispatch::UI::AttrGraph>("disruption-result-regular-stats")->overlap.color = success ? GREEN : RED;
			} else {
				layout.updateSharedData("disruption-description", disruption.description);
				std::vector<std::string> options;
				for (auto& option : disruption.options) {
					options.push_back(option.name);
					layout.updateSharedData(std::format("disruption-options-{}-name", option.name), option.type == Disruption::Option::HERO ? option.disabled ? "???" : option.hero : option.attribute);
					layout.updateSharedData(std::format("disruption-options-{}-disabled", option.name), option.disabled);
				}
				layout.updateSharedData("disruption-options", options);
			}
			layout["main-disruption"]->visible = !result;
			layout["main-disruption-result"]->visible = result;
			layout["buttons"]->visible = result;
			layout["right-assigned-stats"]->visible = !result;
			layout["right-choice-made"]->visible = result;
		}
	}
}

AttrMap<int> Mission::getTotalAttributes() const {
	AttrMap<int> totalAttributes;
	for (const auto& hero_name : assignedHeroes) for (const auto& [attr, value] : HeroesHandler::inst()[hero_name].attributes()) totalAttributes[attr] += value;
	return totalAttributes;
}
int Mission::getTotalAttribute(Attribute attr) const {
	int total = 0;
	for (const auto& hero_name : assignedHeroes) total += HeroesHandler::inst()[hero_name].attributes()[attr];
	return total;
}

int Mission::getSuccessChance() const {
	if (disrupted) return 0;

	AttrMap<int> totalAttributes = getTotalAttributes();
	int total = 0;
	int requiredTotal = 0;

	for (const auto& [attr, requiredValue] : requiredAttributes) {
		int heroValue = totalAttributes[attr];
		total += std::min(heroValue, requiredValue);
		requiredTotal += requiredValue;
	}

	if (requiredTotal == 0) return 100;
	return total * 100 / requiredTotal;
}

bool Mission::isSuccessful() const {
	int chance = getSuccessChance();
	int roll = (rand() % 100) + 1;
	return roll <= chance;
}

bool Mission::isDisruptionSuccessful() const {
	if (curDisruption < 0 || curDisruption > (int)disruptions.size()) return true;
	const auto& disruption = disruptions[curDisruption];
	if (disruption.selected_option == -1) return false;
	const auto& option = disruption.options[disruption.selected_option];
	if (option.type == Disruption::Option::ATTRIBUTE && getTotalAttribute(Attribute{option.attribute}) < option.value) return false;
	return true;
}

bool Mission::isMenuOpen() const {
	return status == Mission::SELECTED || status == Mission::REVIEWING_FAILURE || status == Mission::REVIEWING_SUCESS || status == Mission::DISRUPTION_MENU;
}

std::string Mission::statusToString(Status st) {
	if (st == PENDING) return "PENDING";
	if (st == SELECTED) return "SELECTED";
	if (st == TRAVELLING) return "TRAVELLING";
	if (st == PROGRESS) return "PROGRESS";
	if (st == AWAITING_REVIEW) return "AWAITING_REVIEW";
	if (st == REVIEWING_SUCESS) return "REVIEWING_SUCESS";
	if (st == REVIEWING_FAILURE) return "REVIEWING_FAILURE";
	if (st == DONE) return "DONE";
	if (st == MISSED) return "MISSED";
	if (st == DISRUPTION) return "DISRUPTION";
	if (st == DISRUPTION_MENU) return "DISRUPTION_MENU";
	return "?";
}


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

#define WRITE(var)			{#var, inst.var}
#define WRITE2(var, key)	{#key, inst.var}

void Mission::to_json(nlohmann::json& j, const Mission& inst) {
	j = json{
		// WRITE(btnCancel),
		// WRITE(btnStart),
		WRITE(status),
		WRITE(name),
		WRITE(type),
		WRITE(caller),
		WRITE(description),
		WRITE(requirements),
		WRITE(disruptions),
		WRITE(position),
		WRITE(requiredAttributes),
		WRITE(slots),
		WRITE(difficulty),
		// WRITE(curDisruption),
		// WRITE(timeElapsed),
		WRITE(dangerous),
		WRITE(triggered),
		// WRITE(disrupted),
		// WRITE(assignedHeroes),

		{ "success", {
			WRITE2(missionDuration, duration),
			WRITE2(successMsg, message),
			WRITE2(successMissionTime, delay),
			WRITE2(successMission, mission),
		}},
		{ "failure", {
			WRITE2(failureTime, duration),
			WRITE2(failureMsg, message),
			WRITE2(failureMissionTime, delay),
			WRITE2(failureMission, mission),
		}},
	};
}
void Mission::from_json(const nlohmann::json& j, Mission& inst) {
	// READ(j, btnCancel);
	// READ(j, btnStart);
	READ(j, status);
	READREQ(j, name);
	READREQ(j, type);
	READREQ(j, caller);
	READREQ(j, description);
	READREQ(j, requirements);
	READ(j, disruptions);
	READREQ(j, position);
	READREQ2(j, requiredAttributes, attributes);
	READREQ(j, slots);
	READREQ(j, difficulty);
	// READ(j, curDisruption);
	// READ(j, timeElapsed);
	READ(j, dangerous);
	READ(j, triggered);
	// READ(j, disrupted);
	// READ(j, assignedHeroes);

	const auto& suc = j.at("success");
	READ2(suc, missionDuration, duration);
	READ2(suc, successMsg, message);
	READ2(suc, successMissionTime, delay);
	READ2(suc, successMission, mission);

	const auto& fail = j.at("failure");
	READ2(fail, failureTime, duration);
	READ2(fail, failureMsg, message);
	READ2(fail, failureMissionTime, delay);
	READ2(fail, failureMission, mission);

	inst.assignedSlots.resize(inst.slots);
}

void Disruption::to_json(nlohmann::json& j, const Disruption& inst) {
	j = json{
		WRITE(options),
		WRITE(description),
		WRITE(timeout),
		// WRITE(elapsedTime),
		// WRITE(selected_option),
		// WRITE(optionButtons),
	};
}
void Disruption::from_json(const nlohmann::json& j, Disruption& inst) {
	READ(j, options);
	READ(j, description);
	READ(j, timeout);
	// READ(j, elapsedTime);
	// READ(j, selected_option);
	// READ(j, optionButtons);
}

void Disruption::Option::to_json(nlohmann::json& j, const Option& inst) {
	j = json{
		WRITE(name),
		WRITE(hero),
		WRITE(attribute),
		WRITE(value),
		WRITE(type),
		WRITE(disabled),
		{ "success", {
			WRITE2(successMessage, message),
		}},
		{ "failure", {
			WRITE2(failureMessage, message),
		}},
	};
}
void Disruption::Option::from_json(const nlohmann::json& j, Option& inst) {
	READ(j, name);
	READ(j, hero);
	READ(j, attribute);
	READ(j, successMessage);
	READ(j, failureMessage);
	READ(j, value);
	READ(j, type);
	READ(j, disabled);

	if (j.contains("success")) {
		const auto& suc = j.at("success");
		READ2(suc, successMessage, message);
	}

	if (j.contains("failure")) {
		const auto& fail = j.at("failure");
		READ2(fail, failureMessage, message);
	}
}
