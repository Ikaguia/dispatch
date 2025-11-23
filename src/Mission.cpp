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
	const std::string& failureMsg,
	const std::string& failureMission,
	const std::string& successMsg,
	const std::string& successMission,
	const std::vector<std::string>& requirements,
	raylib::Vector2 position,
	const std::map<std::string, int> &attr,
	int slots,
	int difficulty,
	float failureTime,
	float missionDuration,
	float failureMissionTime,
	float successMissionTime,
	bool dangerous
) :
	name{name},
	type{type},
	caller{caller},
	description{description},
	failureMsg{failureMsg},
	failureMission{failureMission},
	successMsg{successMsg},
	successMission{successMission},
	requirements{requirements},
	position{position},
	requiredAttributes{},
	slots{slots},
	difficulty{difficulty},
	failureTime{failureTime},
	missionDuration{missionDuration},
	failureMissionTime{failureMissionTime},
	successMissionTime{successMissionTime},
	dangerous{dangerous}
{
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		requiredAttributes[attribute] = value;
	}
};

Mission::Mission(const JSONish::Node& data) { load(data); }


void Mission::load(const JSONish::Node& data) {
	name = data.get<std::string>("name");
	type = data.get<std::string>("type");
	caller = data.get<std::string>("caller", "UNKNOWN CALLER");
	description = data.get<std::string>("description");

	if (data.has("requirements")) for (auto& req : data["requirements"].arr) requirements.push_back(req.sval);
	else throw std::invalid_argument("Mission 'requirements' cannot be empty");
	if (data.has("attributes")) for (auto& [name, val] : data["attributes"].obj) requiredAttributes[Attribute::fromString(name)] = val.nval;
	else throw std::invalid_argument("Mission 'attributes' cannot be empty");
	if (data.has("position")) {
		auto pos = data["position"];
		position.x = pos.get<float>(0);
		position.y = pos.get<float>(1);
	} else throw std::invalid_argument("Mission 'position' cannot be empty");

	slots = data.get<int>("slots");
	difficulty = data.get<int>("difficulty");

	if (data.has("failure")) {
		auto failure = data["failure"];
		failureTime = failure.get<float>("duration");
		failureMsg = failure.get<std::string>("message", "MISSION FAILED!");
		failureMission = failure.get<std::string>("mission", "");
		failureMissionTime = failure.get<float>("delay", 0);
	} else throw std::invalid_argument("Mission 'failure' cannot be empty");

	if (data.has("success")) {
		auto success = data["success"];
		missionDuration = success.get<float>("duration");
		successMsg = success.get<std::string>("message", "MISSION COMPLETED!");
		successMission = success.get<std::string>("mission", "");
		successMissionTime = success.get<float>("delay", 0);
	} else throw std::invalid_argument("Mission 'success' cannot be empty");

	dangerous = data.get<bool>("dangerous", false);
	triggered = data.get<bool>("triggered", false);

	if (data.has("disruptions")) {
		auto& disrs = data["disruptions"];
		for (auto& disr : disrs.arr) {
			Disruption disruption;
			disruption.description = disr.get<std::string>("description");
			disruption.timeout = disr.get<float>("timeout");
			if (disr.has("options")) {
				auto& opts = disr["options"];
				for (auto& opt : opts.arr) {
					Disruption::Option option;
					option.name = opt.get<std::string>("name");
					auto type = opt.get<std::string>("type");
					if (type == "HERO") {
						option.type = Disruption::Option::HERO;
						option.hero = opt.get<std::string>("hero");
					} else if (type == "ATTRIBUTE") {
						option.type = Disruption::Option::ATTRIBUTE;
						option.attribute = opt.get<std::string>("attribute");
						option.value = opt.get<int>("value", requiredAttributes[Attribute::fromString(option.attribute)]);
					} else throw std::invalid_argument("Disruption option type must be 'HERO' or 'ATTRIBUTE'");
					if (opt.has("success")) {
						auto& suc = opt["success"];
						option.successMessage = suc.get<std::string>("message", "SUCCESS!");
					}
					if (opt.has("failure")) {
						auto& suc = opt["failure"];
						option.failureMessage = suc.get<std::string>("message", "FAILURE!");
					}
					disruption.options.push_back(option);
					disruption.optionButtons.emplace_back();
				}
			} else throw std::invalid_argument("Disruption 'options' cannot be empty");
			disruptions.push_back(disruption);
		}
	}
}

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
	if (newStatus != Mission::PENDING && newStatus != Mission::SELECTED && newStatus != Mission::DISRUPTION && oldStatus != Mission::DISRUPTION) timeElapsed = 0.0f;

	if (oldStatus == Mission::PENDING && newStatus == Mission::SELECTED) {}
	else if (oldStatus == Mission::SELECTED && newStatus == Mission::PENDING) {
		for (auto hero : assignedHeroes) hero->changeStatus(Hero::AVAILABLE, {}, 0.0f);
		assignedHeroes.clear();
	} else if (oldStatus == Mission::SELECTED && newStatus == Mission::TRAVELLING) for (auto hero : assignedHeroes) hero->changeStatus(Hero::TRAVELLING);
	else if (oldStatus == Mission::TRAVELLING && newStatus == Mission::PROGRESS) {}
	else if (oldStatus == Mission::PROGRESS && newStatus == Mission::DISRUPTION) {
		curDisruption++;
		disruptions[curDisruption].elapsedTime = 0.0f;
	} else if (oldStatus == Mission::DISRUPTION && newStatus == Mission::DISRUPTION_MENU) {
		auto& disruption = disruptions[curDisruption];
		for (auto& option : disruption.options) {
			if (option.type == Disruption::Option::HERO) {
				option.disabled = !std::any_of(assignedHeroes.begin(), assignedHeroes.end(), [&](auto hero){ return hero->name == option.hero; });
			} else option.disabled = false;
		}
	} else if (oldStatus == Mission::DISRUPTION && newStatus == Mission::PROGRESS) {
		disrupted = true;
		curDisruption = disruptions.size();
	} else if (oldStatus == Mission::PROGRESS && newStatus == Mission::AWAITING_REVIEW) for (auto hero : assignedHeroes) hero->changeStatus(Hero::RETURNING);
	else if (oldStatus == Mission::AWAITING_REVIEW) Utils::println("Mission {} completed, it was a {}", name, newStatus==Mission::REVIEWING_SUCESS ? "success" : "failure");
	else if (newStatus == Mission::DONE || newStatus == Mission::MISSED) {
		if (oldStatus == Mission::REVIEWING_SUCESS && !disrupted) {
			float successChance = getSuccessChance();
			float chanceMult = 10.0f / std::sqrt(successChance ? successChance : 1);
			float difficultyMult = std::sqrt(difficulty);
			float heroCountDiv = std::sqrt(assignedHeroes.size());
			int exp = 500 * chanceMult * difficultyMult / heroCountDiv;
			for (auto& hero : assignedHeroes) hero->addExp(exp);
			Utils::println("Each hero gained {} exp", exp);
			if (!successMission.empty()) MissionsHandler::inst().addMissionToQueue(successMission, successMissionTime);
		} else if ((oldStatus == Mission::REVIEWING_FAILURE || disrupted) && dangerous) {
			auto hero = Utils::random_element(assignedHeroes);
			hero->wound();
			Utils::println("{} was wounded", hero->name);
			if (!failureMission.empty()) MissionsHandler::inst().addMissionToQueue(failureMission, failureMissionTime);
		}
		for (auto& hero : assignedHeroes) {
			if (hero->status == Hero::AWAITING_REVIEW) hero->changeStatus(Hero::AVAILABLE, {}, 0.0f);
			else if (hero->status == Hero::WORKING) hero->changeStatus(Hero::RETURNING, {}, 0.0f);
			else hero->mission.reset();
		}
	} else {
		Utils::println("Invalid mission status change, from {} to {}", statusToString(oldStatus), statusToString(newStatus));
		throw std::invalid_argument(std::format("Invalid mission status change, from {} to {}", statusToString(oldStatus), statusToString(newStatus)));
	}
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
		auto typeRect = Utils::drawTextAnchored(type, leftRect, Utils::Anchor::topLeft, Dispatch::UI::fontText, Dispatch::UI::textColor, 18, 1, {10.0f, 10.0f});
		auto linePos = Utils::anchorPos(typeRect, Utils::Anchor::topRight, {4.0f, 0.0f});
		for (int i = 1; i <= 5; i++) {
			linePos.DrawLine({leftRect.x + leftRect.width - 4, linePos.y}, (i*i)%3 + 1, Dispatch::UI::bgDrk);
			linePos.y += 5;
		}
		// Image
		raylib::Rectangle imgRect{leftRect.x + 5, typeRect.y + 30, leftRect.width - 10, 100};
		imgRect.Draw(Dispatch::UI::bgDrk);
		imgRect.DrawLines(BLACK);
		// Caller
		raylib::Rectangle callerBox{leftRect.x + 5, imgRect.y + imgRect.height, leftRect.width - 10, 40};
		callerBox.DrawLines(BLACK);
		Utils::drawTextAnchored("Caller: " + caller, callerBox, Utils::Anchor::left, Dispatch::UI::fontText, Dispatch::UI::textColor, 18.0f, 2.0f, {5.0f, 0.0f}, callerBox.width-10.0f);
		// Description
		raylib::Rectangle descriptionBox{leftRect.x + 5, callerBox.y + callerBox.height, leftRect.width - 10, leftRect.y + leftRect.height - (callerBox.y + callerBox.height) - 5};
		descriptionBox.DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt, Dispatch::UI::bgMed);
		descriptionBox.DrawLines(BLACK);
		Utils::drawTextAnchored(description, descriptionBox, Utils::Anchor::center, Dispatch::UI::fontText, Dispatch::UI::textColor, 14.0f, 2.0f, {}, descriptionBox.width);

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
		auto btnsRect = Utils::anchorRect(centerRect, {centerRect.width, 28}, Utils::Anchor::bottom, {0.0f, -2.0f});
		std::vector<std::pair<std::string, raylib::Rectangle&>> btns{{"CLOSE", btnCancel}};
		if (status == Mission::SELECTED) btns.emplace_back("DISPATCH", btnStart);
		auto rects = Utils::splitRect(btnsRect, 1, btns.size(), {2.0f, 0.0f});
		for (int i = 0; i < (int)btns.size(); i++) {
			auto [name, rect] = btns[i];
			rect = rects[i];
			rect.Draw(Dispatch::UI::bgLgt);
			rect.DrawLines(BLACK);
			Utils::drawTextCentered(name, Utils::center(rect), Dispatch::UI::fontText, 18, BLACK);
		}
		if (status == Mission::SELECTED && assignedHeroes.empty()) btnStart.Draw(Fade(Dispatch::UI::bgDrk, 0.6f));

		// Main panel
		raylib::Rectangle mainPanel{centerRect.x, titleRect.y + titleRect.height, centerRect.width, btnsRect.y - (titleRect.y + titleRect.height) - 4};
		mainPanel.DrawLines(BLACK);
		Utils::inset(mainPanel, 2).DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt);
		if (status == Mission::DISRUPTION_MENU) {
			auto& disruption = disruptions[curDisruption];
			auto mainRect = mainPanel;

			Utils::drawTextAnchored(disruption.description, mainRect, Utils::Anchor::top, Dispatch::UI::fontText, Dispatch::UI::textColor, 24, 2, {0.0f, 8.0f}, mainRect.width-8.0f);

			if (disruption.selected_option == -1) {
				btnCancel.Draw(Fade(Dispatch::UI::bgDrk, 0.6f));

				int sz = disruption.options.size();
				raylib::Vector2 totalSize{0.0f, 0.0f};
				for (auto& option : disruption.options) {
					if (totalSize.y != 0) totalSize.y += 20;
					std::string text = std::format("{} ({})", option.name, option.type == Disruption::Option::HERO ? option.disabled ? "???" : option.hero : option.attribute);
					auto size = Dispatch::UI::fontText.MeasureText(text, 16, 2);
					totalSize.x = std::max(totalSize.x, size.x);
					totalSize.y += size.y;
				}
				raylib::Vector2 offset{0.0f, -(totalSize.y/2.0f)};
				for (int i=0; i<sz; i++) {
					auto& option = disruption.options[i];
					auto& button = disruption.optionButtons[i];
					std::string text = std::format("{} ({})", option.name, option.type == Disruption::Option::HERO ? option.disabled ? "???" : option.hero : option.attribute);

					raylib::Rectangle optionButtonText = Utils::positionTextAnchored(text, mainRect, Utils::Anchor::center, Dispatch::UI::fontText, 16, 2, offset, mainRect.width-8.0f);
					button = Utils::inset(optionButtonText, {-8.0f, -4.0f});
					button.x -= (totalSize.x - optionButtonText.width) / 2; button.width = totalSize.x+16;
					button.Draw(Dispatch::UI::bgMed);
					button.DrawLines(BLACK);
					Dispatch::UI::fontText.DrawText(text, optionButtonText.GetPosition(), 16, 2, Dispatch::UI::textColor);
					if (option.disabled) button.Draw(ColorAlpha(GRAY, 0.6));

					offset.y += optionButtonText.height + 20;
				}
			} else {
				auto opt = disruption.options[disruption.selected_option];
				bool success = isDisruptionSuccessful();
				if (opt.type == Disruption::Option::HERO) {
					auto hero = *std::find_if(assignedHeroes.begin(), assignedHeroes.end(), [&](auto& h){ return h->name == opt.hero; });
					raylib::Rectangle heroText = Utils::positionTextAnchored(hero->name, mainRect, Utils::Anchor::center, Dispatch::UI::fontText, 36, 2, {0.0f,0.0f}, mainRect.width-8.0f);
					raylib::Rectangle heroRect = Utils::inset(heroText, {-8.0f, -4.0f});
					heroRect.Draw(Dispatch::UI::bgLgt);
					heroRect.DrawLines(BLACK);
					Dispatch::UI::fontText.DrawText(hero->name, heroText.GetPosition(), 36, 2, Dispatch::UI::textColor);
				} else {
					raylib::Vector2 radarCenter = Utils::center(mainRect) + raylib::Vector2{0.0f, 30.0f};
					auto totalAttributes = getTotalAttributes();
					float radius = 90.0f;
					Utils::drawRadarGraph(radarCenter, radius, {}, Dispatch::UI::textColor, BROWN);
					std::vector<raylib::Vector2> points, pts, val, req;
					Attribute attribute{opt.attribute};
					const int sides = Attribute::COUNT;
					for (int i = 0; i < sides; i++) {
						Attribute attr{i};
						if (attr == attribute) {
							raylib::Vector2 prev, cur, next;
							prev = raylib::Vector2{0.0f, -radius}.Rotate((i-1)*2.0f*PI/sides);
							cur = raylib::Vector2{0.0f, -radius}.Rotate(i*2.0f*PI/sides);
							next = raylib::Vector2{0.0f, -radius}.Rotate((i+1)*2.0f*PI/sides);
							points.push_back(radarCenter + (prev + cur) / 2); pts.push_back((prev + cur) / 2);
							points.push_back(radarCenter);                    pts.push_back(cur);
							points.push_back(radarCenter + (cur + next) / 2); pts.push_back((cur + next) / 2);
						}
						else points.push_back(radarCenter + raylib::Vector2{0.0f, -radius}.Rotate(i*2.0f*PI/sides));
					}
					points.push_back(points.front());
					points.push_back(radarCenter);
					std::reverse(points.begin(), points.end());
					DrawTriangleFan(points.data(), points.size(), ColorAlpha(DARKGRAY, 0.8f));

					for (auto& pt : pts) {
						val.push_back(radarCenter + pt * std::min(10, totalAttributes[attribute]) / 10.0f);
						req.push_back(radarCenter + pt * opt.value / 10.0f);
					}
					val.push_back(val.front());
					req.push_back(req.front());
					val.push_back(radarCenter);
					req.push_back(radarCenter);
					std::reverse(val.begin(), val.end());
					std::reverse(req.begin(), req.end());
					if (success) {
						DrawTriangleFan(val.data(), val.size(), ColorAlpha(ORANGE, 0.5f));
						DrawTriangleFan(req.data(), req.size(), ColorAlpha(GREEN, 0.5f));
					} else {
						DrawTriangleFan(req.data(), req.size(), ColorAlpha(LIGHTGRAY, 0.5f));
						DrawTriangleFan(val.data(), val.size(), ColorAlpha(RED, 0.5f));
					}
					val[2].DrawLine(val[3], ORANGE);
					req[2].DrawLine(req[3], LIGHTGRAY);
					val[3].DrawLine(val[4], ORANGE);
					req[3].DrawLine(req[4], LIGHTGRAY);
				}
				const std::string& text = success ? opt.successMessage : opt.failureMessage;
				raylib::Rectangle resultText = Utils::positionTextAnchored(text, mainRect, Utils::Anchor::bottom, Dispatch::UI::fontText, 16, 2, {0.0f, -14.0f}, mainRect.width-8.0f);
				raylib::Rectangle resultRect = Utils::inset(resultText, {-8.0f, -4.0f});
				resultRect.Draw(Dispatch::UI::bgMed);
				resultRect.DrawLines(BLACK);
				Dispatch::UI::fontText.DrawText(text, resultText.GetPosition(), 16, 2, Dispatch::UI::textColor);
			}
		} else {
			// Radar graph
			auto radarCenter = Utils::anchorPos(mainPanel, Utils::Anchor::center); if (status==Mission::SELECTED) radarCenter.y -= mainPanel.height/7;
			auto totalAttributes = getTotalAttributes();
			float radarRadius = status==Mission::SELECTED ? 60.0f : 90.0f;
			std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attrs{};
			if (!disrupted) attrs.emplace_back(totalAttributes, ORANGE, true);
			if (status != Mission::SELECTED) {
				attrs.emplace_back(requiredAttributes, LIGHTGRAY, true);
				AttrMap<int> intersect; for (auto& [k, v] : requiredAttributes) intersect[k] = std::min(v, totalAttributes[k]);
				if (!disrupted) attrs.emplace_back(intersect, status == Mission::REVIEWING_SUCESS ? GREEN : RED, false);
			}
			Utils::drawRadarGraph(radarCenter, radarRadius, attrs, Dispatch::UI::textColor, BROWN);
			if (disrupted) {
				std::string disruptedText = "Failed to help heroes";
				auto disruptedBox = Utils::positionTextAnchored(disruptedText, mainPanel, Utils::Anchor::center, Dispatch::UI::fontText, 24.0f, 2.0f, {0.0f, 2*radarRadius/3});
				auto _disruptedBox_ = Utils::inset(disruptedBox, {-8.0f, -4.0f});
				_disruptedBox_.Draw(Dispatch::UI::bgMed);
				_disruptedBox_.DrawLines(BLACK);
				Dispatch::UI::fontText.DrawText(disruptedText, disruptedBox.GetPosition(), 24.0f, 2.0f, Dispatch::UI::textColor);
			}
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
				bool success = status == Mission::REVIEWING_SUCESS;
				raylib::Rectangle reviewRect{mainPanel.x + 6, mainPanel.y + mainPanel.height - 35, mainPanel.width - 12, 32};
				raylib::Rectangle chanceRect{mainPanel.x + mainPanel.width/2 - 45, mainPanel.y + mainPanel.height - 72, 90, 34};
				reviewRect.Draw(Dispatch::UI::bgDrk);
				chanceRect.Draw(success ? GOLD : RED);
				Utils::inset(reviewRect, 1).DrawLines(WHITE);
				Utils::inset(chanceRect, 1).DrawLines(WHITE);
				reviewRect.DrawLines(Dispatch::UI::bgDrk);
				chanceRect.DrawLines(Dispatch::UI::bgDrk);
				std::string reviewText = success ? successMsg : failureMsg;
				std::string chanceText = std::format("{}%", getSuccessChance());
				std::vector<std::tuple<std::string, raylib::Font&, int, raylib::Color, int, raylib::Color, float>> chanceTexts = {
					{std::string{"🎯"}, Dispatch::UI::emojiFont, 32, success ? WHITE : Dispatch::UI::bgDrk, 2, raylib::Color{0,0,0,0}, 0.0f},
					{chanceText, Dispatch::UI::defaultFont, 20, success ? WHITE : Dispatch::UI::bgDrk, 2, raylib::Color{0,0,0,0}, 0.0f}
				};
				Utils::drawTextCentered(reviewText, Utils::center(reviewRect), Dispatch::UI::fontText, 20, WHITE);
				Utils::drawTextSequence(chanceTexts, Utils::center(chanceRect), true, true, 2, true);
			}
			centerRect = Utils::inset(centerRect, -4);
		}

		// RIGHT PANEL
		raylib::Rectangle rightRect{centerRect.x + centerRect.width + 10, leftRect.y, 210, 320};
		raylib::Rectangle{rightRect.GetPosition() + raylib::Vector2{5,5}, rightRect.GetSize()}.Draw(Dispatch::UI::shadow);
		rightRect.Draw(Dispatch::UI::bgMed);

		// Title
		std::string title = status == Mission::DISRUPTION_MENU ? disruptions[curDisruption].selected_option == -1 ? "Assigned Heroes Stats" : "Selected Option" : "Requirements";
		raylib::Vector2 rightTitlePos = rightRect.GetPosition() + raylib::Vector2{10, 5};
		Dispatch::UI::fontText.DrawText(title, rightTitlePos, 18, 1, Dispatch::UI::textColor);
		linePos = raylib::Vector2{rightTitlePos.x + Dispatch::UI::fontText.MeasureText(title, 18, 1).x + 4, rightRect.y + 10};
		for (int i = 1; i <= 5; i++) {
			linePos.DrawLine({rightRect.x + rightRect.width - 4, linePos.y}, (i*i*i)%3 + 1, Dispatch::UI::bgDrk);
			linePos.y += 5;
		}

		// Content Rect
		raylib::Rectangle rightContentRec = Utils::inset(rightRect, {4, 30}); rightContentRec.y += 10;
		rightContentRec.DrawLines(BLACK);
		Utils::inset(rightContentRec, 2).DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt);

		if (status == Mission::DISRUPTION_MENU) {
			auto& disruption = disruptions[curDisruption];
			if (disruption.selected_option == -1) {
				// Assigned Heroes Stats
				auto radarCenter = Utils::anchorPos(rightContentRec, Utils::Anchor::center);
				std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attrs{{getTotalAttributes(), ORANGE, true}};
				Utils::drawRadarGraph(radarCenter, 60.0f, attrs, Dispatch::UI::textColor, BROWN);
			} else {
				// Selected Option
				int sz = disruption.options.size();
				raylib::Vector2 totalSize{0.0f, 0.0f};
				for (auto& option : disruption.options) {
					if (totalSize.y != 0) totalSize.y += 20;
					std::string text = std::format("{} ({})", option.name, option.type == Disruption::Option::HERO ? option.disabled ? "???" : option.hero : option.attribute);
					auto size = Dispatch::UI::fontText.MeasureText(text, 16, 2);
					totalSize.x = std::max(totalSize.x, size.x);
					totalSize.y += size.y;
				}
				raylib::Vector2 offset{0.0f, -(totalSize.y/2.0f)};
				for (int i=0; i<sz; i++) {
					auto& option = disruption.options[i];
					auto& button = disruption.optionButtons[i];
					std::string text = std::format("{} ({})", option.name, option.type == Disruption::Option::HERO ? option.hero : option.attribute);

					raylib::Rectangle optionButtonText = Utils::positionTextAnchored(text, rightContentRec, Utils::Anchor::center, Dispatch::UI::fontText, 16, 2, offset, mainRect.width-8.0f);
					button = Utils::inset(optionButtonText, {-8.0f, -4.0f});
					button.x -= (totalSize.x - optionButtonText.width) / 2; button.width = totalSize.x+16;
					button.Draw(i == disruption.selected_option ? GOLD : Dispatch::UI::bgMed);
					button.DrawLines(BLACK);
					Dispatch::UI::fontText.DrawText(text, optionButtonText.GetPosition(), 16, 2, Dispatch::UI::textColor);
					if (i != disruption.selected_option) button.Draw(ColorAlpha(GRAY, 0.6));

					offset.y += optionButtonText.height + 20;
				}
			}
		} else {
			// Requirements
			std::vector<std::pair<std::string, float>> reqs;
			for (const auto& req : requirements) reqs.emplace_back(req, 0.0f);
			float indentSize = Dispatch::UI::fontText.MeasureText("> ", 16, 1).x, totalH=0;
			for (auto& [txt, h] : reqs) {
				txt = Utils::addLineBreaks(txt, rightContentRec.width - 8 - indentSize, Dispatch::UI::fontText, 16, 1);
				h = Dispatch::UI::fontText.MeasureText(txt, 16, 1).y;
				if (totalH) totalH += 10;
				totalH += h;
			}
			// raylib::Vector2 reqPos = {rightContentRec.x + 4 + indentSize, rightContentRec.y + (rightContentRec.height - totalH) / 2 + 4 };
			auto reqPos = Utils::anchorPos(rightContentRec, Utils::Anchor::left, {indentSize+4.0f, -totalH/2.0f + 4.0f});
			for (auto& [txt, h] : reqs) {
				Dispatch::UI::fontText.DrawText("> ", {reqPos.x - indentSize, reqPos.y}, 16, 1, Dispatch::UI::textColor);
				Dispatch::UI::fontText.DrawText(txt, reqPos, 16, 1, Dispatch::UI::textColor);
				reqPos.y += h + 10;
			}
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
		} else if (status == Mission::DISRUPTION) {
			if (mousePos.CheckCollision(position, 28)) changeStatus(Mission::DISRUPTION_MENU);
		} else if (status == Mission::DISRUPTION_MENU) {
			auto& disruption = disruptions[curDisruption];
			if (disruption.selected_option == -1) {
				int sz = static_cast<int>(disruption.options.size());
				for (int idx = 0; idx < sz; idx++) {
					auto& option = disruption.options[idx];
					auto& button = disruption.optionButtons[idx];
					if (!option.disabled && mousePos.CheckCollision(button)) disruption.selected_option = idx;
				}
			} else if (mousePos.CheckCollision(btnCancel)) {
				disrupted |= !isDisruptionSuccessful();
				if (++curDisruption == (int)disruptions.size()) changeStatus(Mission::DONE);
			}
		}
	}
}

AttrMap<int> Mission::getTotalAttributes() const {
	AttrMap<int> totalAttributes{};
	for (const auto& hero : assignedHeroes) for (const auto& [attr, value] : hero->attributes()) totalAttributes[attr] += value;
	return totalAttributes;
}
int Mission::getTotalAttribute(Attribute attr) const {
	int total = 0;
	for (const auto& hero : assignedHeroes) total += hero->attributes()[attr];
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