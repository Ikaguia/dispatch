#include <exception>

#include <MissionsHandler.hpp>
#include <Utils.hpp>

MissionsHandler::MissionsHandler() {}

MissionsHandler& MissionsHandler::inst() {
	static MissionsHandler singleton;
	return singleton;
}

Mission& MissionsHandler::addRandomMission(int difficulty, int slots) {
	static int missionCount = 0;
	difficulty = difficulty == -1 ? Utils::randInt(1, 5) : difficulty;
	auto res = active_missions.emplace(new Mission{
		// name
		"Random Mission " + std::to_string(++missionCount),
		// description
		"A randomly generated mission.",
		// position
		{ (float)Utils::randInt(50, 900), (float)Utils::randInt(50, 350) },
		// required attributes
		std::map<std::string, int>{
			{"com", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 12)},
			{"vig", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 12)},
			{"mob", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 12)},
			{"int", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 12)},
			{"cha", Utils::clamp(Utils::randInt(1, 10) * (5 + difficulty) / 10, 1, 12)}
		},
		// slots
		slots == -1 ? Utils::clamp(difficulty * Utils::randInt(10, 15) / 10, 1, 4) : slots,
		// failure time
		(float)Utils::randInt(10, 60),
		// mission duration
		20.0f,
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

	timeToNext -= deltaTime;
	if (timeToNext <= 0) {
		addRandomMission();
		timeToNext = rand() % 6 + rand() % 6 + 2;
	}
}
