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

	raylib::RenderTexture2D target{window.GetWidth(), window.GetHeight()};
	raylib::Shader crtShader{"", "resources/shaders/crt-monitor.fs"};
	raylib::Texture background{"resources/images/background.png"};
	int timeLoc = GetShaderLocation(crtShader, "time");
	SetTargetFPS(60);
	srand(time(nullptr));

	HeroesHandler& heroesHandler = HeroesHandler::inst();
	MissionsHandler& missionsHandler = MissionsHandler::inst();
	CityMap& cityMap = CityMap::inst();
	std::string paused = "";

	while (!window.ShouldClose()) {
		float deltaTime = GetFrameTime();

		if (missionsHandler.paused()) paused = "mission";
		else if (heroesHandler.paused()) paused = "hero";
		else paused = "";

		bool handled = heroesHandler.handleInput();
		if (!handled) missionsHandler.handleInput();

		if (paused == "") {
			cityMap.update(deltaTime);
			heroesHandler.update(deltaTime);
			missionsHandler.update(deltaTime);
		}
		float t = GetTime();
		crtShader.SetValue(timeLoc, &t, SHADER_UNIFORM_FLOAT);

		if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
			raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
			std::cout << "mousePos: " << mousePos.x << "," << mousePos.y << std::endl;
		}


		target.BeginMode();
			ClearBackground(BLACK);
			background.Draw({0, 0}, 0, 1.0f * window.GetWidth() / background.GetWidth());

			if (paused == "hero") {
				// cityMap.renderUI();
				missionsHandler.renderUI();
				heroesHandler.renderUI();
			} else {
				// cityMap.renderUI();
				heroesHandler.renderUI();
				missionsHandler.renderUI();
			}
		target.EndMode();

		BeginDrawing();
			window.ClearBackground(BLACK);
			crtShader.BeginMode();
				DrawTextureRec(
					target.texture,
					Rectangle{0, 0, (float)target.texture.width, -(float)target.texture.height},
					Vector2{0, 0},
					WHITE
				);
			crtShader.EndMode();
		EndDrawing();
	}

	return 0;
}
