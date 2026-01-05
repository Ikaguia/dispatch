#include <raylib-cpp.hpp>
#include <iostream>
#include <string>

#include <Console.hpp>
#include <Hero.hpp>
#include <Attribute.hpp>
#include <MissionsHandler.hpp>
#include <HeroesHandler.hpp>
#include <TextureManager.hpp>
#include <CityMap.hpp>
#include <UI.hpp>

// raylib::Window window(1920, 1080, "raylib-cpp - basic window"); float bgScale;
raylib::Window window(960, 540, "raylib-cpp - basic window"); float bgScale;

int main() {
	AttachConsole();
	try {
		SetTargetFPS(60);

		raylib::RenderTexture2D target{window.GetWidth(), window.GetHeight()};
		raylib::Texture background{"resources/images/background.png"}; bgScale = 1.0f * window.GetWidth() / background.GetWidth();
		HeroesHandler& heroesHandler = HeroesHandler::inst();
		MissionsHandler& missionsHandler = MissionsHandler::inst();
		TextureManager& textureManager = TextureManager::inst();
		CityMap& cityMap = CityMap::inst();
		std::string paused = "";

		// CRT Monitor Shader
		raylib::Shader crtShader{0, "resources/shaders/crt-monitor.fs"};
		int crtTimeLoc = GetShaderLocation(crtShader, "time");
		// Clouds Shader
		raylib::Rectangle mapRect{65, 50, 1790, 731};
		raylib::Rectangle scaledMapRect{65*bgScale, 50*bgScale, 1790*bgScale, 731*bgScale};
		raylib::Shader cloudShader{0, "resources/shaders/clouds.fs"};
		int cloudsTimeLoc = cloudShader.GetLocation("time");
		int cloudsResLoc = cloudShader.GetLocation("resolution");
		int cloudsSpeedLoc = cloudShader.GetLocation("speed");
		int cloudsDensityLoc = cloudShader.GetLocation("density");
		cloudShader.SetValue(cloudsResLoc, (float[2]){ scaledMapRect.width, scaledMapRect.height }, SHADER_UNIFORM_VEC2);
		cloudShader.SetValue(cloudsSpeedLoc, (float[1]){ 0.03f }, SHADER_UNIFORM_FLOAT);
		cloudShader.SetValue(cloudsDensityLoc, (float[1]){ 0.5f }, SHADER_UNIFORM_FLOAT);
		raylib::Texture dummy(raylib::Image{1,1,WHITE});

		while (!window.ShouldClose()) {
			float deltaTime = GetFrameTime();

			if (missionsHandler.paused()) paused = "mission";
			else if (heroesHandler.paused()) paused = "hero";
			else paused = "";

			bool handled = heroesHandler.handleInput();
			if (paused != "hero" && !handled) missionsHandler.handleInput();

			if (paused == "") {
				cityMap.update(deltaTime);
				heroesHandler.update(deltaTime);
				missionsHandler.update(deltaTime);
			}
			float t = GetTime();
			crtShader.SetValue(crtTimeLoc, &t, SHADER_UNIFORM_FLOAT);
			cloudShader.SetValue(cloudsTimeLoc, &t, SHADER_UNIFORM_FLOAT);

			if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
				raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
				std::cout << "mousePos: " << mousePos.x << "," << mousePos.y << std::endl;
			}

			target.BeginMode();
				ClearBackground(BLACK);
				background.Draw({0, 0}, 0, bgScale);

				cloudShader.BeginMode();
					DrawTexturePro(
						dummy,
						{0,0,1,1},
						scaledMapRect,
						{0,0},
						0.0f,
						WHITE
					);
				cloudShader.EndMode();

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
				// crtShader.BeginMode();
					DrawTextureRec(
						target.texture,
						Rectangle{0, 0, (float)target.texture.width, -(float)target.texture.height},
						Vector2{0, 0},
						WHITE
					);
				// crtShader.EndMode();
			EndDrawing();
		}

		missionsHandler.missions.clear();
		heroesHandler.heroes.clear();
		textureManager.clear();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		throw e;
	}

	return 0;
}
