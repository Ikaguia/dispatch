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
	enum Tabs {
		UPGRADE,
		POWERS,
		INFO
	};
	Tabs tab = UPGRADE;
	std::map<std::string, raylib::Rectangle> btns;

	static HeroesHandler& inst();

	std::weak_ptr<const Hero> operator[](const std::string& name) const;
	std::weak_ptr<Hero> operator[](const std::string& name);

	bool paused() const;

	void renderUI();
	bool handleInput();
	void update(float deltaTime);

};
