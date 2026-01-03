#include <exception>
#include <algorithm>
#include <memory>
#include <format>
#include <fstream>

#include <nlohmann/json.hpp>
using nlohmann::json;

#include <MissionsHandler.hpp>
#include <Utils.hpp>
#include <Attribute.hpp>

MissionsHandler::MissionsHandler() {
	loadMissions("resources/data/missions/test.json");
	// loadMissions("resources/data/missions/Missions1.json");
	// loadMissions("resources/data/missions/Missions2.json");
	// loadMissions("resources/data/missions/Missions3.json");
}

MissionsHandler& MissionsHandler::inst() {
	static MissionsHandler singleton;
	return singleton;
}


void MissionsHandler::loadMissions(const std::string& file) {
	Utils::println("Loading missions from {}", file);
	json missions_array = Utils::readJsonFile(file);
	if (!missions_array.is_array()) throw std::runtime_error("Heroes error: Top-level JSON must be an array of heroes.");
	Utils::println("Read {} missions", missions_array.size());
	for (auto& data : missions_array) {
		auto ms = std::make_unique<Mission>(data);
		// Utils::println("Loaded {}mission '{}'", ms->triggered ? "triggered " : "", ms->name);
		if (ms->triggered) trigger.insert(ms->name);
		else loaded.insert(ms->name);
		missions[ms->name] = std::move(ms);
	}
}
Mission& MissionsHandler::activateMission() {
	if (loaded.empty()) return createRandomMission();
	auto& name = Utils::random_element(loaded);
	return activateMission(name);
}
Mission& MissionsHandler::activateMission(const std::string& name) {
	if (!missions.contains(name)) throw std::invalid_argument("Cannot activate mission that is not loaded");
	if (active.contains(name)) throw std::invalid_argument("Cannot activate active mission");
	if (previous.contains(name)) throw std::invalid_argument("Cannot activate completed mission");
	auto& mission = (*this)[name];
	active.insert(name);
	loaded.erase(name);
	trigger.erase(name);
	return mission;
}
Mission& MissionsHandler::createRandomMission(int difficulty, int slots) {
	static int missionCount = 0;
	difficulty = difficulty == -1 ? Utils::randInt(1, 5) : difficulty;
	std::unordered_map<std::string, int> attributes{
		{"com", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 10)},
		{"vig", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 10)},
		{"mob", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 10)},
		{"int", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 10)},
		{"cha", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 10)}
	};
	std::vector<std::pair<Attribute, int>> sorted; for (const auto& [k, v] : attributes) sorted.emplace_back(Attribute::fromString(k), v);
	std::sort(sorted.begin(), sorted.end(), [&](auto& kv1, auto& kv2){ return kv1.second > kv2.second; });
	std::vector<std::string> requirements;
	for (auto& [attr, val] : sorted) if (requirements.size() <3) requirements.push_back(std::format("{} {} {}", attr.toIcon(), val > 6 ? "High" : val > 3 ? "Medium" : "Low", attr.toString()));

	std::string name = "Random Mission " + std::to_string(++missionCount);
	auto mission = std::make_unique<Mission>(
		// name
		name,
		// type
		std::vector<std::string>{"Rescue", "Assault", "Recon", "Escort", "Sabotage"}[Utils::randInt(0,4)],
		// caller
		std::vector<std::string>{"Agency Alpha", "Bravo Corp", "Charlie Ops", "Delta Force", "Echo Unit"}[Utils::randInt(0,4)],
		// description
		"A randomly generated mission.",
		// failure message
		"MISSION FAILED!",
		// failure mission
		"",
		// success message
		"MISSION COMPLETED!",
		// success mission
		"",
		// requirements
		requirements,
		// position
		raylib::Vector2{ (float)Utils::randInt(50, 900), (float)Utils::randInt(50, 350) },
		// required attributes
		attributes,
		// slots
		slots == -1 ? Utils::clamp(difficulty * Utils::randInt(10, 15) / 10, 1, 4) : slots,
		// difficulty
		difficulty,
		// failure time
		(float)Utils::randInt(10, 60),
		// mission duration
		20.0f,
		// failure mission time
		0.0f,
		// success mission time
		0.0f,
		// dangerous
		(difficulty >= 3) ? true : ((rand()%5) < difficulty)
	);
	active.insert(name);
	missions[name] = std::move(mission);
	return *missions[name].get();
}

const Mission& MissionsHandler::operator[](const std::string& name) const { return *(missions.at(name).get()); }
Mission& MissionsHandler::operator[](const std::string& name) { return *(missions.at(name).get()); }

bool MissionsHandler::paused() const { return !selected.empty(); }

void MissionsHandler::selectMission(const std::string& name) {
	if (!active.count(name)) return;
	selected = name;
	(*this)[name].setupLayout(layoutMissionDetails);
}

void MissionsHandler::unselectMission() { selected.clear(); }

void MissionsHandler::addMissionToQueue(const std::string& name, float time) {
	if (!missions.contains(name)) throw std::invalid_argument("Mission must be loaded");
	Utils::println("Mission {} scheduled in {} seconds", name, time);
	mission_queue.emplace_back(name, time);
}


void MissionsHandler::renderUI() {
	for (auto& name : active) (*this)[name].renderUI(false);
	if (paused()) layoutMissionDetails.render();
}

void MissionsHandler::handleInput() {
	if (paused())	{
		auto& mission = (*this)[selected];
		layoutMissionDetails.handleInput();
		mission.handleInput();
		if (!mission.isMenuOpen()) unselectMission();
	} else for (auto& name : active) {
		auto& mission = (*this)[name];
		mission.handleInput();
		if (mission.isMenuOpen()) {
			selectMission(name);
			break;
		}
	}
}

void MissionsHandler::update(float deltaTime) {
	std::unordered_set<std::string> finished;

	if (paused()) (*this)[selected].update(deltaTime);
	else for (auto& name : active) {
		auto& mission = (*this)[name];
		mission.update(deltaTime);
		if (mission.status == Mission::DONE || mission.status == Mission::MISSED) previous.insert(name);
	}

	for (auto& name : finished) {
		active.erase(name);
		previous.insert(name);
	}

	timeToNext -= deltaTime / (1 + active.size());
	if (timeToNext <= 0) {
		activateMission();
		timeToNext = rand() % 4 + rand() % 4 + 2;
	}

	for (int i = 0; i < (int)mission_queue.size(); i++) {
		auto& [name, time] = mission_queue[i];
		if (time < deltaTime) {
			activateMission(name);
			mission_queue.erase(mission_queue.begin()+i--);
		} else time -= deltaTime;
	}
}
