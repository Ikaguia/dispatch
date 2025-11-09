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
	return *active_missions.emplace_back(new Mission{
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
		// travel duration
		10.0f,
		// mission duration
		20.0f,
		// dangerous
		(difficulty >= 3) ? true : ((rand()%5) < difficulty)
	});
}

bool MissionsHandler::paused() const { return selectedMissionIndex != -1; }

void MissionsHandler::selectMission(int index) {
	if (index < 0 || index >= static_cast<int>(active_missions.size())) return;
	selectedMissionIndex = index;
}

void MissionsHandler::unselectMission() {
	selectedMissionIndex = -1;
}

void MissionsHandler::renderUI() {
	for (auto& mission : active_missions) mission->renderUI(false);
	if (selectedMissionIndex != -1) active_missions[selectedMissionIndex]->renderUI(true);
}

void MissionsHandler::handleInput() {
	if (selectedMissionIndex != -1)	{
		Mission& mission = *active_missions[selectedMissionIndex];
		mission.handleInput();
		if (mission.status != Mission::SELECTED) selectedMissionIndex = -1;
	}
	else for (int i = 0; i < static_cast<int>(active_missions.size()); ++i) {
		Mission& mission = *active_missions[i];
		mission.handleInput();
		if (mission.status == Mission::SELECTED) {
			selectedMissionIndex = i;
			break;
		}
	}
}

void MissionsHandler::update(float deltaTime) {
	if (selectedMissionIndex != -1) active_missions[selectedMissionIndex]->update(deltaTime);
	else for (int i = 0; i < static_cast<int>(active_missions.size()); ++i) {
		std::shared_ptr<Mission> mission = active_missions[i];
		mission->update(deltaTime);
		if ((mission->status == Mission::COMPLETED || mission->status == Mission::FAILED || mission->status == Mission::MISSED) && mission->timeElapsed >= 10.0f) {
			previous_missions.push_back(mission);
			active_missions.erase(active_missions.begin() + i);
			--i;
		}
	}

	timeToNext -= deltaTime;
	if (timeToNext <= 0) {
		addRandomMission();
		timeToNext = rand() % 6 + 5;
	}
}
