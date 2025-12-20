#pragma once

#include <vector>
#include <string>
#include <memory>

#include <Hero.hpp>
#include <Mission.hpp>
#include <UI.hpp>

class HeroesHandler {
private:
	HeroesHandler();
public:
	std::vector<std::shared_ptr<Hero>> loaded_heroes;
	std::vector<std::shared_ptr<Hero>> active_heroes;
	std::vector<std::shared_ptr<Hero>> previous_heroes;
	raylib::Rectangle detailsTabButton{895, 166, 30, 32};
	int selectedHeroIndex = -1;
	enum Tabs {
		UPGRADE,
		POWERS,
		INFO
	};
	Tabs tab = UPGRADE;
	std::map<std::string, raylib::Rectangle> btns;
	Dispatch::UI::Layout layoutHeroDetails{"resources/layouts/hero-details.json"};

	static HeroesHandler& inst();
	void loadHeroes(const std::string& filePath, bool activate=false);

	std::weak_ptr<const Hero> operator[](const std::string& name) const;
	std::weak_ptr<Hero> operator[](const std::string& name);

	bool paused() const;
	bool isHeroSelected(const std::weak_ptr<Hero> hero) const;

	void renderUI();
	bool handleInput();
	void update(float deltaTime);
	void selectHero(int idx);
	void changeTab(Tabs newTab);

	std::string tabToString(Tabs t);
};
