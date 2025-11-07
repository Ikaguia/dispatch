#include <raylib-cpp.hpp>
#include <iostream>
#include <string>

#include <Console.hpp>
#include <Hero.hpp>
#include <Attribute.hpp>
#include <MissionsHandler.hpp>
#include <HeroesHandler.hpp>

int main() {
	AttachConsole();

	int screenWidth = 960;
	int screenHeight = 540;
	raylib::Window window(screenWidth, screenHeight, "raylib-cpp - basic window");
	raylib::Texture background("assets/background.png");
	SetTargetFPS(60);
	srand(time(nullptr));

	HeroesHandler& heroesHandler = HeroesHandler::inst();
	MissionsHandler& missionsHandler = MissionsHandler::inst();
	missionsHandler.addRandomMission(1);
	missionsHandler.addRandomMission(2);
	missionsHandler.addRandomMission(3);
	missionsHandler.addRandomMission(4);
	missionsHandler.addRandomMission(5);

	float previousTime = GetTime();
	while (!window.ShouldClose()) {
		float deltaTime = GetTime() - previousTime;
		previousTime = GetTime();

		if (missionsHandler.selectedMissionIndex != -1) {
			Mission& selectedMission = *missionsHandler.active_missions[missionsHandler.selectedMissionIndex];
			if (!heroesHandler.handleInput(selectedMission)) missionsHandler.handleInput();
		} else {
			heroesHandler.handleInput();
			missionsHandler.handleInput();
		}
		heroesHandler.update(deltaTime);
		missionsHandler.update(deltaTime);

		BeginDrawing();

		window.ClearBackground(RAYWHITE);

		// Object methods.
		background.Draw(
			raylib::Vector2{0, 0},
			0,
			1.0f * screenWidth / background.GetWidth()
		);

		heroesHandler.renderUI();
		missionsHandler.renderUI();

		EndDrawing();
	}

	return 0;
}
