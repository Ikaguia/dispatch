#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <Hero.hpp>
#include <Mission.hpp>
#include <UI.hpp>

class HeroesHandler {
private:
	HeroesHandler();
	raylib::Rectangle detailsTabButton{895, 166, 30, 32};
public:
	std::unordered_map<std::string, std::unique_ptr<Hero>> heroes;
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
	bool isHeroSelected(const std::string& name) const;

	void renderUI();
	bool handleInput();
	void update(float deltaTime);
	void selectHero(const std::string& name);
	void changeTab(Tab newTab);
};
