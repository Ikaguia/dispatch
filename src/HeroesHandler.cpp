#include <string>
#include <algorithm>
#include <HeroesHandler.hpp>
#include <Hero.hpp>
#include <Mission.hpp>

HeroesHandler::HeroesHandler() {
	active_heroes.emplace_back(new Hero{"Sonar", {
		{"combat", 1},
		{"vigor", 2},
		{"mobility", 4},
		{"charisma", 3},
		{"intelligence", 5}
	}});
	active_heroes.emplace_back(new Hero{"Flambae", {
		{"combat", 4},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 2},
		{"intelligence", 1}
	}});
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
	}});
}

HeroesHandler& HeroesHandler::inst() {
	static HeroesHandler singleton;
	return singleton;
}


std::shared_ptr<const Hero> HeroesHandler::operator[](const std::string& name) const {
	auto it = std::find_if(active_heroes.begin(), active_heroes.end(), [&](const std::shared_ptr<Hero>& hero) { return hero->name == name; });
	if (it != active_heroes.end()) return std::const_pointer_cast<const Hero>(*it);
	return nullptr;
}
std::shared_ptr<Hero> HeroesHandler::operator[](const std::string& name) {
	auto it = std::find_if(active_heroes.begin(), active_heroes.end(), [&](const std::shared_ptr<Hero>& hero) { return hero->name == name; });
	if (it != active_heroes.end()) return *it;
	return nullptr;
}


void HeroesHandler::renderUI() {
	// // for (auto& hero : active_heroes) hero->renderUI();
	// if (selectedHeroIndex != -1) {
	// 	DrawText(("Selected Hero: " + active_heroes[selectedHeroIndex]->name).c_str(), 190, 200, 20, LIGHTGRAY);
	// 	DrawText(("  Combat: " + std::to_string(active_heroes[selectedHeroIndex]->attributes().[Attribute::COMBAT])).c_str(), 190, 230, 20, LIGHTGRAY);
	// 	DrawText(("  Vigor: " + std::to_string(active_heroes[selectedHeroIndex]->attributes().[Attribute::VIGOR])).c_str(), 190, 260, 20, LIGHTGRAY);
	// 	DrawText(("  Mobility: " + std::to_string(active_heroes[selectedHeroIndex]->attributes().[Attribute::MOBILITY])).c_str(), 190, 290, 20, LIGHTGRAY);
	// 	DrawText(("  Charisma: " + std::to_string(active_heroes[selectedHeroIndex]->attributes().[Attribute::CHARISMA])).c_str(), 190, 320, 20, LIGHTGRAY);
	// 	DrawText(("  Intelligence: " + std::to_string(active_heroes[selectedHeroIndex]->attributes().[Attribute::INTELLIGENCE])).c_str(), 190, 350, 20, LIGHTGRAY);
	// } else {
	// 	DrawText("No Hero Selected", 190, 200, 20, DARKGRAY);
	// }
	raylib::Vector2 pos{125, 400};
	for (auto [idx, hero] : Utils::enumerate(active_heroes)) {
		hero->renderUI(pos);
		pos.x += 103;
	}
}

void HeroesHandler::handleInput() {
	// for (auto& hero : active_heroes) hero->handleInput();
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		if (mousePos.y >= 400 && mousePos.y <= 515) {
			int idx = ((int)mousePos.x - 120) / 100;
			int rem = ((int)mousePos.x - 120) % 100;
			if (rem <= 90 && idx >=0 && idx < (int)active_heroes.size()) selectedHeroIndex = idx;
		} else selectedHeroIndex = -1;
	}
}

bool HeroesHandler::handleInput(Mission& selectedMission) {
	// for (auto& hero : active_heroes) hero->handleInput();
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		if (mousePos.y >= 400 && mousePos.y <= 515) {
			int idx = ((int)mousePos.x - 125) / 103;
			int rem = ((int)mousePos.x - 125) % 103;
			if (rem <= 90 && idx >=0 && idx < (int)active_heroes.size()) {
				selectedHeroIndex = idx;
				selectedMission.toggleHero(active_heroes[selectedHeroIndex]);
				return true;
			}
		} else selectedHeroIndex = -1;
	}
	return false;
}

void HeroesHandler::update(float deltaTime) {
	for (auto& hero : active_heroes) hero->update(deltaTime);
}
