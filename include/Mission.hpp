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
		MISSED
	};

	std::string name, type, caller, description;
	std::vector<std::string> requirements;
	raylib::Vector2 position{0.0f, 0.0f};
	AttrMap<int> requiredAttributes{};
	int slots;
	int difficulty = 1;
	float failureTime = 60.0f;
	float missionDuration = 20.0f;
	bool dangerous = false;

	std::set<std::shared_ptr<Hero>> assignedHeroes{};
	Status status = Mission::PENDING;
	float timeElapsed = 0.0f;

	Mission(const std::string& name, const std::string& type, const std::string& caller, const std::string& description, const std::vector<std::string>& requirements, raylib::Vector2 pos, const std::map<std::string,int> &attr, int slots, int difficulty, float failureTime, float missionDuration, bool dangerous);
	Mission(const std::string& fileName);
	Mission(const Mission&) = delete;
	Mission& operator=(const Mission&) = delete;

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
};
