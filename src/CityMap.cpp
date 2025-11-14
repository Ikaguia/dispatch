#include <fstream>
#include <queue>
#include <map>

#include <CityMap.hpp>
#include <Utils.hpp>

extern raylib::Window window;

CityMap::CityMap(std::string fileName) { load(fileName); }

CityMap& CityMap::inst() {
	static CityMap singleton;
	return singleton;
}


void CityMap::load(std::string fileName) {
	Utils::println("Loading {}", fileName);
	std::ifstream file{fileName};
	int n, m, k;
	file >> n >> sourceSize.x >> sourceSize.y;
	raylib::Vector2 scaling{window.GetWidth() / sourceSize.x, window.GetHeight() / sourceSize.y};
	points.resize(n);
	roads.resize(n);
	for (int i = 0; i < n; i++) {
		auto& point = points[i];
		file >> point.x >> point.y >> m;
		point.x *= scaling.x;
		point.y *= scaling.y;
		Utils::println("Loaded point {}: {},{}", i, point.x, point.y);
		for (int j = 0; j < m; j++) {
			file >> k;
			roads[i].insert(k);
			roads[k].insert(i);
		}
	}
	Utils::println("Loaded {} with {} points", fileName, n);
}
void CityMap::renderUI() {
	int sz = static_cast<int>(points.size());
	for (int i = 0; i < sz; i++) {
		raylib::Vector2 src = points[i];
		raylib::Color srcColor{static_cast<unsigned int>(i * static_cast<int>(src.x) * static_cast<int>(src.y))};
		srcColor = srcColor.Alpha(1.0f);
		for (int j : roads[i]) {
			raylib::Vector2 dest = points[j];
			raylib::Color destColor{static_cast<unsigned int>(i * static_cast<int>(dest.x) * static_cast<int>(dest.y))};
			destColor = destColor.Alpha(1.0f);
			Utils::drawLineGradient(src, dest, srcColor, destColor, 20);
		}
	}
	for (int i = 0; i < sz; i++) {
		raylib::Vector2 src = points[i];
		raylib::Color srcColor{static_cast<unsigned int>(i * static_cast<int>(src.x) * static_cast<int>(src.y))};
		srcColor = srcColor.Alpha(1.0f);
		src.DrawCircle(12, srcColor);
		Utils::drawTextCentered(std::to_string(i), src, 12, WHITE);
	}
}
void CityMap::update(float elapsedTime) {}

int CityMap::closestPoint(raylib::Vector2 p) {
	int mnIdx = 0, sz = static_cast<int>(points.size());
	float mnDst = p.Distance(points[0]);
	for (int i = 1; i < sz; i++) {
		float dst = p.Distance(points[i]);
		if (dst < mnDst) {
			mnDst = dst;
			mnIdx = i;
		}
	}
	return mnIdx;
}
int CityMap::shortestPath(raylib::Vector2 src, raylib::Vector2 dest) { return shortestPath(closestPoint(src), closestPoint(dest)); }
int CityMap::shortestPath(int src, int dest) {
	static std::map<std::pair<int,int>, int> route;

	if (!route.count({src, dest})) {
		int sz = static_cast<int>(points.size());
		std::vector<std::pair<int, float>> best;
		std::queue<int> to_visit;
		for (int i = 0; i < sz; i++) best.emplace_back(-1, 100000000);
		to_visit.push(dest);
		best[dest] = {dest, 0};
		while(!to_visit.empty()) {
			int cur = to_visit.front();
			to_visit.pop();
			for(int adj : roads[cur]) {
				float dst = best[cur].second + points[cur].Distance(points[adj]);
				if (best[adj].first == -1 || dst < best[adj].second) {
					best[adj] = {cur, dst};
					to_visit.push(adj);
				}
			}
		}
		for (int i = 0; i < sz; i++) {
			auto [path, dist] = best[i];
			if (path != -1) route[{i, dest}] = path;
		}
	}

	return route[{src, dest}];
}
