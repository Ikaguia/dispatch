#pragma once

#include <vector>
#include <string>
#include <memory>
#include <Mission.hpp>
#include <Hero.hpp>

class MissionsHandler {
private:
	MissionsHandler();
public:
	std::vector<std::shared_ptr<Mission>> active_missions;
	std::vector<std::shared_ptr<Mission>> previous_missions;
	int selectedMissionIndex = -1;

	static MissionsHandler& inst();

	Mission& addRandomMission(int difficulty=-1, int slots=-1);

	void selectMission(int index);
	void unselectMission();

	void renderUI();
	void handleInput();
	void update(float deltaTime);
};
