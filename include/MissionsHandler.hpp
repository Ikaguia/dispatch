#pragma once

#include <vector>
#include <string>
#include <Mission.hpp>
#include <Hero.hpp>

class MissionsHandler {
public:
	std::vector<Mission> active_missions;
	std::vector<Mission> previous_missions;
	int selectedMissionIndex = -1;

	MissionsHandler();
	Mission& addRandomMission(int difficulty=-1, int slots=-1);

	void selectMission(int index);
	void unselectMission();

	void renderUI();
	void handleInput();
	void update(float deltaTime);
};
