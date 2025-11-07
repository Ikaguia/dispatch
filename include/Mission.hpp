#pragma once

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <memory>

#include <raylib-cpp.hpp>
#include <Attribute.hpp>
#include <Hero.hpp>

class Mission {
public:
	enum Status {
		PENDING,
		SELECTED,
		TRAVELLING,
		PROGRESS,
		COMPLETED,
		FAILED,
		MISSED
	};

	std::string name;
	std::string description;
	Vector2 position{0.0f, 0.0f};
	AttrMap<int> requiredAttributes{};
	int slots;
	float failureTime = 60.0f;
	float travelDuration = 10.0f;
	float missionDuration = 20.0f;

	std::set<std::shared_ptr<Hero>> assignedHeroes{};
	Status status = PENDING;
	float timeElapsed = 0.0f;

	Mission(const std::string& name, const std::string& description, Vector2 pos, const std::map<std::string,int> &attr, int slots, float failureTime, float travelDuration, float missionDuration);
	Mission(const Mission&) = delete;
	Mission& operator=(const Mission&) = delete;

	void toggleHero(std::shared_ptr<Hero> hero);
	void assignHero(std::shared_ptr<Hero> hero);
	void unassignHero(std::shared_ptr<Hero> hero);

	void start();
	void changeStatus(Status newStatus);
	void update(float deltaTime);

	void renderUI(bool full=false) const;
	void handleInput();

	AttrMap<int> getTotalAttributes() const;
	int getSuccessChance() const;
	bool isSuccessful() const;
};
