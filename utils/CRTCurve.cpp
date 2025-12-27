#include <raylib-cpp.hpp>
#include <string>
#include <CRTCurve.hpp>

extern raylib::Window window;

int crtCurve(std::string inputPath, std::string outputPath, bool warp, raylib::Vector2 curveStrength, int iterations) {
	// load source texture
	raylib::Texture srcTex(inputPath);
	raylib::Rectangle srcRect{0.0f, 0.0f, static_cast<float>(srcTex.GetWidth()), static_cast<float>(srcTex.GetHeight())};
	raylib::Rectangle srcRectFlipped = srcRect; srcRectFlipped.height *= -1;

	// create render target with same size
	raylib::RenderTexture2D rt(srcRect.width, srcRect.height);

	// load shaders (ensure paths correct)
	raylib::Shader shader(0, warp ? "resources/shaders/crt-curvature.fs" : "resources/shaders/crt-unwarp.fs");

	// locations
	shader.SetValue(shader.GetLocation("curveStrength"), &curveStrength, SHADER_UNIFORM_VEC2);
	if (!warp) shader.SetValue(shader.GetLocation("iterations"), &iterations, SHADER_UNIFORM_INT);

	// ----- Render curved / unwrapped image -----
	rt.BeginMode();
		window.ClearBackground(BLANK);
		shader.BeginMode();
			// draw srcTex full area -> shader will sample texture0
			DrawTexturePro(
				srcTex,
				srcRectFlipped,
				Rectangle{0, 0, srcRect.width, srcRect.height},
				Vector2{0, 0},
				0,
				WHITE
			);

		shader.EndMode();
	rt.EndMode();

	// Read back and save edited image
	raylib::Image img(rt.texture); // GPU -> CPU
	img.Export(outputPath);

	// Image img = LoadImageFromTexture(rt.texture);
	// ImageFlipVertical(&img);
	// ExportImage(img, outputPath.c_str());
	// UnloadImage(img);

	while (!window.ShouldClose()) {
		BeginDrawing();
			DrawTexturePro(
				rt.texture,
				srcRect,
				Rectangle{0, 0, static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight())},
				Vector2{0, 0},
				0,
				WHITE
			);
		EndDrawing();
	}

	return 0;
}
