#pragma once

#include <vector>
#include <string>
#include <memory>
#include <Hero.hpp>
#include <Mission.hpp>

class HeroesHandler {
public:
	std::vector<std::shared_ptr<Hero>> active_heroes;
	std::vector<std::shared_ptr<Hero>> previous_heroes;
	int selectedHeroIndex = -1;

	HeroesHandler();

	void renderUI();
	void handleInput();
	bool handleInput(Mission& selectedMission);
	void update(float deltaTime);
};
