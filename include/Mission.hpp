#pragma once

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <memory>

class Mission;

#include <raylib-cpp.hpp>
#include <Attribute.hpp>
#include <Hero.hpp>
#include <JSONish.hpp>

class Disruption {
public:
	struct Option {
		std::string name, hero, attribute, successMessage, failureMessage;
		int value;
		enum Type { HERO, ATTRIBUTE } type;
		bool disabled = false;
	};
	std::vector<Option> options;
	std::string description;
	float timeout, elapsedTime;
	int selected_option = -1;
	std::vector<raylib::Rectangle> optionButtons;
};

class Mission : public std::enable_shared_from_this<Mission> {
private:
	raylib::Rectangle btnCancel, btnStart;
public:
	enum Status {
		PENDING,
		SELECTED,
		TRAVELLING,
		PROGRESS,
		AWAITING_REVIEW,
		REVIEWING_SUCESS,
		REVIEWING_FAILURE,
		DONE,
		MISSED,
		DISRUPTION,
		DISRUPTION_MENU
	};

	std::string name, type, caller, description, failureMsg="MISSION FAILED", failureMission, successMsg="MISSION COMPLETED", successMission;
	std::vector<std::string> requirements;
	std::vector<Disruption> disruptions;
	raylib::Vector2 position{0.0f, 0.0f};
	AttrMap<int> requiredAttributes{};
	int slots;
	int difficulty = 1;
	float failureTime = 60.0f;
	float missionDuration = 20.0f;
	float failureMissionTime = 0.0f;
	float successMissionTime = 0.0f;
	bool dangerous = false;
	bool triggered = false;
	bool disrupted = false;
	int curDisruption = -1;

	std::set<std::shared_ptr<Hero>> assignedHeroes{};
	Status status = Mission::PENDING;
	float timeElapsed = 0.0f;

	Mission(const std::string& name, const std::string& type, const std::string& caller, const std::string& description, const std::string& failureMsg, const std::string& failureMission, const std::string& successMsg, const std::string& successMission, const std::vector<std::string>& requirements, raylib::Vector2 pos, const std::map<std::string,int> &attr, int slots, int difficulty, float failureTime, float missionDuration, float failureMissionTimeool, float successMissionTime, bool dangerous);
	Mission(const JSONish::Node& data);
	Mission(const Mission&) = delete;
	Mission& operator=(const Mission&) = delete;

	void load(const JSONish::Node& data);
	void validate() const;

	void toggleHero(std::shared_ptr<Hero> hero);
	void assignHero(std::shared_ptr<Hero> hero);
	void unassignHero(std::shared_ptr<Hero> hero);

	void changeStatus(Status newStatus);
	void update(float deltaTime);

	void renderUI(bool full=false);
	void handleInput();

	AttrMap<int> getTotalAttributes() const;
	int getSuccessChance() const;
	bool isSuccessful() const;
	bool isMenuOpen() const;

	static std::string statusToString(Status st);
};
