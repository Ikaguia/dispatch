#pragma once

#include <set>
#include <string>
#include <memory>
#include <Mission.hpp>
#include <Hero.hpp>

class MissionsHandler {
private:
	MissionsHandler();
public:
	std::set<std::shared_ptr<Mission>> loaded_missions;
	std::set<std::shared_ptr<Mission>> active_missions;
	std::set<std::shared_ptr<Mission>> previous_missions;
	std::weak_ptr<Mission> selectedMission;
	float timeToNext = 1.0f;

	static MissionsHandler& inst();

	void loadMissions(const std::string& file);
	void loadMissions(std::ifstream& input);
	Mission& activateMission();
	Mission& activateMission(std::weak_ptr<Mission> mission);
	Mission& createRandomMission(int difficulty=-1, int slots=-1);

	bool paused() const;

	void selectMission(std::weak_ptr<Mission> ms);
	void unselectMission();

	void renderUI();
	void handleInput();
	void update(float deltaTime);
};
