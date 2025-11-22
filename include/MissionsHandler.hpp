#pragma once

#include <set>
#include <map>
#include <string>
#include <memory>
#include <Mission.hpp>
#include <Hero.hpp>

class MissionsHandler {
private:
	MissionsHandler();
public:
	std::map<std::string, std::shared_ptr<Mission>> trigger_missions;
	std::map<std::string, std::shared_ptr<Mission>> loaded_missions;
	std::set<std::shared_ptr<Mission>> active_missions;
	std::set<std::shared_ptr<Mission>> previous_missions;
	std::weak_ptr<Mission> selectedMission;
	std::vector<std::pair<std::weak_ptr<Mission>,float>> mission_queue;
	float timeToNext = 1.0f;

	static MissionsHandler& inst();

	void loadMissions(const std::string& file);
	Mission& activateMission();
	Mission& activateMission(std::string mission_name);
	Mission& activateMission(std::weak_ptr<Mission> mission);
	Mission& createRandomMission(int difficulty=-1, int slots=-1);

	bool paused() const;

	void selectMission(std::weak_ptr<Mission> ms);
	void unselectMission();

	void addMissionToQueue(std::string mission_name, float time);
	void addMissionToQueue(std::weak_ptr<Mission> mission, float time);

	void renderUI();
	void handleInput();
	void update(float deltaTime);
};
