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
	raylib::Rectangle detailsTabButton{895, 166, 30, 32};
	std::map<std::string, std::unique_ptr<Hero>> loaded;
public:
	std::vector<std::string> roster;
	std::string selected;
	Dispatch::UI::Layout layoutHeroDetails{"resources/layouts/hero-details.json"};
	enum Tab {
		UPGRADE,
		POWERS,
		INFO
	} tab = UPGRADE;

	static HeroesHandler& inst();
	void loadHeroes(const std::string& filePath, bool activate=false);

	const Hero& operator[](const std::string& name) const;
	Hero& operator[](const std::string& name);

	bool paused() const;
	bool isHeroSelected(const Hero& hero) const;

	void renderUI();
	bool handleInput();
	void update(float deltaTime);
	void selectHero(const std::string& name);
	void changeTab(Tab newTab);
};
