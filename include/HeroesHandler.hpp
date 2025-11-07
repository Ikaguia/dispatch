#pragma once

#include <vector>
#include <string>
#include <memory>
#include <Hero.hpp>
#include <Mission.hpp>

class HeroesHandler {
private:
	HeroesHandler();
public:
	std::vector<std::shared_ptr<Hero>> active_heroes;
	std::vector<std::shared_ptr<Hero>> previous_heroes;
	int selectedHeroIndex = -1;

	static HeroesHandler& inst();

	std::shared_ptr<const Hero> operator[](const std::string& name) const;
	std::shared_ptr<Hero> operator[](const std::string& name);

	void renderUI();
	void handleInput();
	bool handleInput(Mission& selectedMission);
	void update(float deltaTime);
};
