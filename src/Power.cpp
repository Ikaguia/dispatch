#include <Power.hpp>

Power::Power(const JSONish::Node& data, bool unlock) {
	name = data.get<std::string>("name");
	description = data.get<std::string>("description");

	unlocked = data.get<bool>("unlocked", unlock);
	flight = data.get<bool>("flight", false);
}
