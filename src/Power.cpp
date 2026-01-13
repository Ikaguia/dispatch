#include <format>
#include <iostream>

#include <Power.hpp>
#include <Effect.hpp>

using nlohmann::json;

void Power::to_json(json& j) const {
	j = json{
		{"name", name},
		{"description", description},
		{"unlocked", unlocked},
		{"effects", json::array()}
	};
	for (auto& effect : effects) {
		json je;
		effect->to_json(je);
		j["effects"].push_back(je);
	}
}
void Power::from_json(const json& j) {
	if (j.is_object()) {
		j.at("name").get_to(name);
		description = j.value("description", description);
		unlocked = j.value("unlocked", unlocked);
		if (j.contains("effects")) {
			auto& jes = j["effects"];
			effects.reserve(jes.size());
			for (auto& je : jes) {
				auto effect = Effect::effect_factory(je, hero, this);
				effects.push_back(std::move(effect));
			}
		}
	} else throw std::runtime_error("Invalid format for Power");
}
