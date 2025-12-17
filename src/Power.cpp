#include <Power.hpp>
using nlohmann::json;

Power::Power(const json& data, bool unlock) {
	name = data.at("name").get<std::string>();
	description = data.at("description").get<std::string>();

	unlocked = data.value("unlocked", unlock);
	flight = data.value("flight", false);
}
