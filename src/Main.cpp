#include <raylib-cpp.hpp>
#include <iostream>
#include <string>

#include <Console.hpp>
#include <Hero.hpp>
#include <Attribute.hpp>

std::vector<Hero> initHeroes() {
	std::vector<Hero> heroes;
	heroes.emplace_back(Hero{"Sonar", {{"int", 5}, {"mob", 7}}});
	heroes.emplace_back(Hero{"Flambae", {{"com", 8}, {"cha", 6}}});
	heroes.emplace_back(Hero{"Invisigal", {{"mob", 9}, {"int", 4}}});
	heroes.emplace_back(Hero{"Punch Up", {{"com", 10}, {"vig", 8}}});
	heroes.emplace_back(Hero{"Prism", {{"int", 9}, {"cha", 7}}});
	heroes.emplace_back(Hero{"Malevola", {{"cha", 10}, {"int", 6}}});
	heroes.emplace_back(Hero{"Golem", {{"com", 9}, {"vig", 10}}});
	heroes.emplace_back(Hero{"Coupé", {{"mob", 8}, {"com", 6}}});
	return heroes;
}

int main() {
	AttachConsole();

	int screenWidth = 960;
	int screenHeight = 540;

	raylib::Window window(screenWidth, screenHeight, "raylib-cpp - basic window");
	raylib::Texture background("assets/background.png");
	auto heroes = initHeroes();
	int selectedHero = -1;
	SetTargetFPS(60);

	while (!window.ShouldClose()) {
		if (raylib::Mouse::IsButtonReleased(MOUSE_BUTTON_LEFT)) {
			Vector2 mousePos = raylib::Mouse::GetPosition();
			if (mousePos.y >= 400 && mousePos.y <= 515) {
				int idx = ((int)mousePos.x - 120) / 100;
				int rem = ((int)mousePos.x - 120) % 100;
				if (rem <= 90 && idx >=0 && idx < (int)heroes.size()) {
					std::cout << heroes[idx].name << std::endl;
					std::cout << "  Combat: " << heroes[idx].attributes[Attribute::COMBAT] << std::endl;
					std::cout << "  Vigor: " << heroes[idx].attributes[Attribute::VIGOR] << std::endl;
					std::cout << "  Mobility: " << heroes[idx].attributes[Attribute::MOBILITY] << std::endl;
					std::cout << "  Charisma: " << heroes[idx].attributes[Attribute::CHARISMA] << std::endl;
					std::cout << "  Intelligence: " << heroes[idx].attributes[Attribute::INTELLIGENCE] << std::endl;
					selectedHero = idx;
				}
			} else {
				std::cout << "Mouse X: " << GetMouseX() << ", Mouse Y: " << GetMouseY() << std::endl;
				selectedHero = -1;
			}
		}

		BeginDrawing();

		window.ClearBackground(RAYWHITE);

		// Object methods.
		background.Draw(
			raylib::Vector2{0, 0},
			0,
			1.0f * screenWidth / background.GetWidth()
		);

		if (selectedHero != -1) {
			DrawText(("Selected Hero: " + heroes[selectedHero].name).c_str(), 190, 200, 20, LIGHTGRAY);
			DrawText(("  Combat: " + std::to_string(heroes[selectedHero].attributes[Attribute::COMBAT])).c_str(), 190, 230, 20, LIGHTGRAY);
			DrawText(("  Vigor: " + std::to_string(heroes[selectedHero].attributes[Attribute::VIGOR])).c_str(), 190, 260, 20, LIGHTGRAY);
			DrawText(("  Mobility: " + std::to_string(heroes[selectedHero].attributes[Attribute::MOBILITY])).c_str(), 190, 290, 20, LIGHTGRAY);
			DrawText(("  Charisma: " + std::to_string(heroes[selectedHero].attributes[Attribute::CHARISMA])).c_str(), 190, 320, 20, LIGHTGRAY);
			DrawText(("  Intelligence: " + std::to_string(heroes[selectedHero].attributes[Attribute::INTELLIGENCE])).c_str(), 190, 350, 20, LIGHTGRAY);
		} else {
			DrawText("No Hero Selected", 190, 200, 20, DARKGRAY);
		}

		EndDrawing();
	}

	return 0;
}
