#include <MissionsHandler.hpp>

MissionsHandler::MissionsHandler() {}

MissionsHandler& MissionsHandler::inst() {
	static MissionsHandler singleton;
	return singleton;
}

Mission& MissionsHandler::addRandomMission(int difficulty, int slots) {
	static int missionCount = 0;
	return *active_missions.emplace_back(new Mission{
		"Random Mission " + std::to_string(++missionCount),
		"A randomly generated mission.",
		Vector2{ rand() % 800 + 100.0f, rand() % 400 + 50.0f },
		std::map<std::string, int>{
			{"com", ((rand() % 10) + 1) * (5 + (difficulty == -1 ? 0 : difficulty)) / 10},
			{"vig", ((rand() % 10) + 1) * (5 + (difficulty == -1 ? 0 : difficulty)) / 10},
			{"mob", ((rand() % 10) + 1) * (5 + (difficulty == -1 ? 0 : difficulty)) / 10},
			{"int", ((rand() % 10) + 1) * (5 + (difficulty == -1 ? 0 : difficulty)) / 10},
			{"cha", ((rand() % 10) + 1) * (5 + (difficulty == -1 ? 0 : difficulty)) / 10}
		},
		slots == -1 ? ((rand() % 4) + 1) : slots,
		10.0f + (rand()%50),
		10.0f,
		20.0f
	});
}

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
}
