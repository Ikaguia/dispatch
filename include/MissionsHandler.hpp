#pragma once

#include <set>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <string>
#include <memory>
#include <Mission.hpp>
#include <Hero.hpp>

class MissionsHandler {
private:
	MissionsHandler();
public:
	Dispatch::UI::Layout layoutMissionDetails{"resources/layouts/mission-details.json"};
	std::unordered_map<std::string, std::unique_ptr<Mission>> missions;
	std::unordered_set<std::string> trigger, loaded, active, previous;
	std::string selected;
	std::vector<std::pair<std::string,float>> mission_queue;
	float timeToNext = 1.0f;

	static MissionsHandler& inst();

	void loadMissions(const std::string& file);
	Mission& activateMission();
	Mission& activateMission(const std::string& name);
	Mission& createRandomMission(int difficulty=-1, int slots=-1);

	const Mission& operator[](const std::string& name) const;
	Mission& operator[](const std::string& name);

	bool paused() const;

	void selectMission(const std::string& name);
	void unselectMission();

	void addMissionToQueue(const std::string& name, float time);

	void renderUI();
	void handleInput();
	void update(float deltaTime);
};
