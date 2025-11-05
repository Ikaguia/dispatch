#pragma once

#include <string>
#include <map>
#include <vector>

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
	AttrMap<int> requiredAttributes{};
	std::vector<Hero> assignedHeroes{};
	Vector2 position{0.0f, 0.0f};
	float timeElapsed = 0.0f;
	float failureTime = 60.0f;
	float travelDuration = 10.0f;
	float missionDuration = 20.0f;
	Status status = PENDING;

	Mission(const std::string& name, const std::string& description, Vector2 pos, const std::map<std::string,int> &attr, float failureTime, float travelDuration, float missionDuration);

	void assignHero(const Hero& hero);
	void unassignHero(const Hero& hero);
	void unassignHero(const std::string& heroName);

	void start();
	void changeStatus(Status newStatus);
	void update(float deltaTime);

	void renderUI(bool full=false) const;
	void handleInput();

	AttrMap<int> getTotalAttributes() const;
	int getSuccessChance() const;
	bool isSuccessful() const;
};
