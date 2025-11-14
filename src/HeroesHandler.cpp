#include <string>
#include <algorithm>

#include <Utils.hpp>
#include <Mission.hpp>
#include <Hero.hpp>
#include <HeroesHandler.hpp>
#include <MissionsHandler.hpp>

HeroesHandler::HeroesHandler() {
	active_heroes.emplace_back(new Hero{"Sonar", {
		{"combat", 1},
		{"vigor", 2},
		{"mobility", 4},
		{"charisma", 3},
		{"intelligence", 5}
	}, true});
	active_heroes.emplace_back(new Hero{"Flambae", {
		{"combat", 4},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 2},
		{"intelligence", 1}
	}, true});
	active_heroes.emplace_back(new Hero{"Invisigal", {
		{"combat", 2},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 1},
		{"intelligence", 2}
	}});
	active_heroes.emplace_back(new Hero{"Punch Up", {
		{"combat", 3},
		{"vigor", 4},
		{"mobility", 1},
		{"charisma", 3},
		{"intelligence", 1}
	}});
	active_heroes.emplace_back(new Hero{"Prism", {
		{"combat", 3},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 4},
		{"intelligence", 2}
	}});
	active_heroes.emplace_back(new Hero{"Malevola", {
		{"combat", 3},
		{"vigor", 2},
		{"mobility", 2},
		{"charisma", 3},
		{"intelligence", 2}
	}});
	active_heroes.emplace_back(new Hero{"Golem", {
		{"combat", 2},
		{"vigor", 4},
		{"mobility", 1},
		{"charisma", 2},
		{"intelligence", 1}
	}});
	active_heroes.emplace_back(new Hero{"Coupé", {
		{"combat", 4},
		{"vigor", 2},
		{"mobility", 3},
		{"charisma", 1},
		{"intelligence", 2}
	}, true});
}

HeroesHandler& HeroesHandler::inst() {
	static HeroesHandler singleton;
	return singleton;
}


std::weak_ptr<const Hero> HeroesHandler::operator[](const std::string& name) const {
	auto it = std::find_if(active_heroes.begin(), active_heroes.end(), [&](const std::shared_ptr<Hero>& hero) { return hero->name == name; });
	if (it != active_heroes.end()) return std::const_pointer_cast<const Hero>(*it);
	return {};
}
std::weak_ptr<Hero> HeroesHandler::operator[](const std::string& name) {
	auto it = std::find_if(active_heroes.begin(), active_heroes.end(), [&](const std::shared_ptr<Hero>& hero) { return hero->name == name; });
	if (it != active_heroes.end()) return *it;
	return {};
}


void HeroesHandler::renderUI() {
	raylib::Vector2 pos{125, 400};
	for (auto hero : active_heroes) {
		hero->renderUI(pos);
		pos.x += 103;
	}
}

bool HeroesHandler::handleInput() {
	auto mission = MissionsHandler::inst().selectedMission;
	auto missionStatus = mission.expired() ? Mission::DONE : mission.lock()->status;
	if (missionStatus == Mission::SELECTED && raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		if (mousePos.y >= 400 && mousePos.y <= 515) {
			int idx = ((int)mousePos.x - 125) / 103;
			int rem = ((int)mousePos.x - 125) % 103;
			if (rem <= 90 && idx >=0 && idx < (int)active_heroes.size()) {
				selectedHeroIndex = idx;
				mission.lock()->toggleHero(active_heroes[selectedHeroIndex]);
				return true;
			}
		} else selectedHeroIndex = -1;
	}
	return false;
}

void HeroesHandler::update(float deltaTime) { for (auto& hero : active_heroes) hero->update(deltaTime); }
