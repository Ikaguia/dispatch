#include <raylib-cpp.hpp>
#include <iostream>
#include <string>

#include <Console.hpp>
#include <Hero.hpp>
#include <Attribute.hpp>
#include <MissionsHandler.hpp>
#include <HeroesHandler.hpp>
#include <CityMap.hpp>

raylib::Window window(960, 540, "raylib-cpp - basic window");

int main() {
	AttachConsole();

	raylib::Texture background("resources/images/background.png");
	SetTargetFPS(60);
	srand(time(nullptr));

	HeroesHandler& heroesHandler = HeroesHandler::inst();
	MissionsHandler& missionsHandler = MissionsHandler::inst();
	CityMap& cityMap = CityMap::inst();
	bool paused = false;

	while (!window.ShouldClose()) {
		float deltaTime = GetFrameTime();

		paused = missionsHandler.paused();

		bool handled = heroesHandler.handleInput();
		if (!handled) missionsHandler.handleInput();

		if (!paused) {
			cityMap.update(deltaTime);
			heroesHandler.update(deltaTime);
			missionsHandler.update(deltaTime);
		}

		if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
			raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
			std::cout << "mousePos: " << mousePos.x << "," << mousePos.y << std::endl;
		}

		BeginDrawing();

		window.ClearBackground(RAYWHITE);
		background.Draw({0, 0}, 0, 1.0f * window.GetWidth() / background.GetWidth());

		// cityMap.renderUI();
		heroesHandler.renderUI();
		missionsHandler.renderUI();

		EndDrawing();
	}

	return 0;
}
