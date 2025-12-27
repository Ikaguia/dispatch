#pragma once
#include <string>
#include <raylib-cpp.hpp>

int crtCurve(std::string inputPath, std::string outputPath="resources/images/output.png", bool warp=false, raylib::Vector2 curveStrength={0.04f,0.04f}, int iterations=8);
