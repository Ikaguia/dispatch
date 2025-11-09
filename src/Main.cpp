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
	raylib::Texture background("resources/images/background.png");
	SetTargetFPS(60);
	srand(time(nullptr));

	HeroesHandler& heroesHandler = HeroesHandler::inst();
	MissionsHandler& missionsHandler = MissionsHandler::inst();

	float previousTime = GetTime();
	while (!window.ShouldClose()) {
		float deltaTime = GetTime() - previousTime;
		previousTime = GetTime();

		if (missionsHandler.paused()) deltaTime = 0;

		if (missionsHandler.selectedMissionIndex != -1) {
			Mission& selectedMission = *missionsHandler.active_missions[missionsHandler.selectedMissionIndex];
			if (!heroesHandler.handleInput(selectedMission)) missionsHandler.handleInput();
		} else {
			heroesHandler.handleInput();
			missionsHandler.handleInput();
		}
		heroesHandler.update(deltaTime);
		missionsHandler.update(deltaTime);

		if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
			raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
			std::cout << "mousePos: " << mousePos.x << "," << mousePos.y << std::endl;
		}

		BeginDrawing();

		window.ClearBackground(RAYWHITE);

		// Object methods.
		background.Draw({0, 0}, 0, 1.0f * screenWidth / background.GetWidth());
		heroesHandler.renderUI();
		missionsHandler.renderUI();

		EndDrawing();
	}

	return 0;
}
