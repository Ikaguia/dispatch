#pragma once

#include <vector>
#include <set>
#include <raylib-cpp.hpp>

class CityMap {
private:
	CityMap(std::string fileName="resources/data/map-graph.txt");
public:
	static CityMap& inst();

	std::vector<raylib::Vector2> points;
	std::vector<std::set<int>> roads;
	raylib::Vector2 sourceSize;

	void load(std::string fileName);
	void renderUI();
	void update(float elapsedTime);

	int closestPoint(raylib::Vector2 p);
	int shortestPath(raylib::Vector2 src, raylib::Vector2 dest);
	int shortestPath(int src, int dest);
};
