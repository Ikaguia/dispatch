#include <exception>
#include <algorithm>
#include <memory>
#include <format>
#include <fstream>

#include <MissionsHandler.hpp>
#include <Utils.hpp>
#include <Attribute.hpp>
#include <JSONish.hpp>

MissionsHandler::MissionsHandler() {
	loadMissions("resources/data/missions/test.txt");
	// loadMissions("resources/data/missions/Missions1.txt");
	// loadMissions("resources/data/missions/Missions2.txt");
	// loadMissions("resources/data/missions/Missions3.txt");

	// JSONish::Parser parser(Utils::readFile("resources/data/missions/test.jsonish"));
	// std::cout << parser.parseArray().toString() << std::endl;
}

MissionsHandler& MissionsHandler::inst() {
	static MissionsHandler singleton;
	return singleton;
}


void MissionsHandler::loadMissions(const std::string& file) {
	Utils::println("Loading missions from {}", file);
	std::ifstream input(file);
	loadMissions(input);
}
void MissionsHandler::loadMissions(std::ifstream& input) {
	std::string header;
	bool triggered;
	std::getline(input, header);
	while (input.peek() != EOF) {
		auto mission = std::make_shared<Mission>(input, header);
		input >> triggered; input.ignore();
		if (triggered) trigger_missions[mission->name] = mission;
		else loaded_missions[mission->name] = mission;
	}
}
Mission& MissionsHandler::activateMission() {
	if (loaded_missions.empty()) return createRandomMission();
	auto mission = Utils::random_element(loaded_missions).second;
	return activateMission(mission->weak_from_this());
}
Mission& MissionsHandler::activateMission(std::string mission_name) {
	if (trigger_missions.count(mission_name)) return activateMission(trigger_missions[mission_name]);
	return activateMission(loaded_missions[mission_name]);
}
Mission& MissionsHandler::activateMission(std::weak_ptr<Mission> mission) {
	auto sharedMission = mission.lock();
	if (!sharedMission) throw std::invalid_argument("Cannot activate expired mission");
	bool triggered = trigger_missions.count(sharedMission->name);
	if (!triggered && !loaded_missions.count(sharedMission->name)) throw std::invalid_argument("Cannot activate mission that is not loaded");
	if (active_missions.count(sharedMission)) throw std::invalid_argument("Cannot activate mission that is already active");
	active_missions.insert(sharedMission);
	if (triggered) trigger_missions.erase(sharedMission->name);
	else loaded_missions.erase(sharedMission->name);
	return *sharedMission;
}
Mission& MissionsHandler::createRandomMission(int difficulty, int slots) {
	static int missionCount = 0;
	difficulty = difficulty == -1 ? Utils::randInt(1, 5) : difficulty;
	std::map<std::string, int> attributes{
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

	auto res = active_missions.emplace(new Mission{
		// name
		"Random Mission " + std::to_string(++missionCount),
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
		{ (float)Utils::randInt(50, 900), (float)Utils::randInt(50, 350) },
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
	});
	if (res.second == true) return **(res.first);
	throw std::runtime_error("Failed to construct mission");
}

bool MissionsHandler::paused() const { return !selectedMission.expired(); }

void MissionsHandler::selectMission(std::weak_ptr<Mission> ms) {
	if (!active_missions.count(ms.lock())) return;
	selectedMission = ms;
}

void MissionsHandler::unselectMission() { selectedMission.reset(); }

void MissionsHandler::addMissionToQueue(std::string mission_name, float time) {
	if (trigger_missions.count(mission_name)) addMissionToQueue(trigger_missions[mission_name], time);
	else addMissionToQueue(loaded_missions[mission_name], time);
}
void MissionsHandler::addMissionToQueue(std::weak_ptr<Mission> mission, float time) {
	if (mission.expired()) throw std::invalid_argument("Mission must be loaded");
	Utils::println("Mission {} scheduled in {} seconds", mission.lock()->name, time);
	mission_queue.emplace_back(mission, time);
}


void MissionsHandler::renderUI() {
	for (auto& mission : active_missions) mission->renderUI(false);
	if (!selectedMission.expired()) selectedMission.lock()->renderUI(true);
}

void MissionsHandler::handleInput() {
	if (!selectedMission.expired())	{
		Mission& mission = *selectedMission.lock();
		mission.handleInput();
		if (mission.status != Mission::SELECTED && mission.status != Mission::REVIEWING_SUCESS && mission.status != Mission::REVIEWING_FAILURE) selectedMission.reset();
	} else for (auto& mission : active_missions) {
		mission->handleInput();
		if (mission->status == Mission::SELECTED || mission->status == Mission::REVIEWING_SUCESS || mission->status == Mission::REVIEWING_FAILURE) {
			selectedMission = mission->weak_from_this();
			break;
		}
	}
}

void MissionsHandler::update(float deltaTime) {
	std::vector<std::shared_ptr<Mission>> finished;

	if (!selectedMission.expired()) selectedMission.lock()->update(deltaTime);
	else for (auto& mission : active_missions) {
		mission->update(deltaTime);
		if (mission->status == Mission::DONE || mission->status == Mission::MISSED) finished.push_back(mission);
	}

	for (auto& mission : finished) {
		active_missions.erase(mission);
		previous_missions.insert(mission);
	}

	timeToNext -= deltaTime / (1 + active_missions.size());
	if (timeToNext <= 0) {
		activateMission();
		timeToNext = rand() % 4 + rand() % 4 + 2;
	}

	for (int i = 0; i < (int)mission_queue.size(); i++) {
		auto& [mission, time] = mission_queue[i];
		if (time < deltaTime) {
			activateMission(mission);
			mission_queue.erase(mission_queue.begin()+i--);
		} else time -= deltaTime;
	}
}
