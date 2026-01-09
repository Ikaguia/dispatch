#include <format>

#include <Power.hpp>
using nlohmann::json;

Power::Power(const json& data, bool unlock) {
	name = data.at("name").get<std::string>();
	description = data.at("description").get<std::string>();

	unlocked = data.value("unlocked", unlock);
	flight = data.value("flight", false);
}

void Power::to_json(nlohmann::json& j) const {
	j = json{
		{"name", name},
		{"description", description},
		{"unlocked", unlocked},
		{"flight", flight},
	};
}
void Power::from_json(const nlohmann::json& j) {
	if (j.is_object()) {
		j.at("name").get_to(name);
		description = j.value("description", description);
		unlocked = j.value("unlocked", unlocked);
		flight = j.value("flight", flight);
	} else throw std::runtime_error("Invalid format for Power");
}

std::unique_ptr<Power> Power::power_factory(const std::string& t) {
	auto type = Utils::toLower(t);
	if (type == "power") return std::make_unique<Power>();
	// else if (type == "aaa") return std::make_unique<aaa>();
	else throw std::runtime_error(std::format("Invalid power type '{}'", type));
}

std::unique_ptr<Power> Power::power_factory(const json& data) {
	const std::string& name = data.at("name");
	// const std::string& type = data.at("type");
	const std::string type = "POWER";
	auto power = power_factory(type);
	try { data.get_to(*power); }
	catch (const std::exception& e) { throw std::runtime_error(std::format("Layout Error: Failed to deserialize power '{}' of type '{}'. {}", name, type, e.what())); }
	return power;
}
