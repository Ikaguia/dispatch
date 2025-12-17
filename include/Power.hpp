#pragma once

#include <string>
#include <nlohmann/json.hpp>

class Power {
public:
	Power()=default;
	Power(const nlohmann::json& data, bool unlock=false);

	std::string name, description;
	bool unlocked=false, flight=false;
};

namespace nlohmann {
	template <>
	struct adl_serializer<Power> {
		static void to_json(json& j, const Power& pwr) {
			j = json{
				{"name", pwr.name},
				{"description", pwr.description},
				{"unlocked", pwr.unlocked},
				{"flight", pwr.flight},
			};
		}
		static void from_json(const json& j, Power& pwr) {
			if (j.is_object()) {
				pwr.name = j.at("name").get<std::string>();
				pwr.description = j.value("description", pwr.description);
				pwr.unlocked = j.value("unlocked", pwr.unlocked);
				pwr.flight = j.value("flight", pwr.flight);
			} else throw std::runtime_error("Invalid format for Power");
		}
	};
}
