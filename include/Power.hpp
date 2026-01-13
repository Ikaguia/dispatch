#pragma once

#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

class Effect;
class Hero;

class Power {
public:
	std::string name, description;
	std::vector<std::shared_ptr<Effect>> effects;
	bool unlocked=false;

	Hero* hero;

	virtual void to_json(nlohmann::json& j) const;
	virtual void from_json(const nlohmann::json& j);
};

inline void to_json(nlohmann::json& j, const Power& inst) { j = nlohmann::json(); inst.to_json(j); }
inline void from_json(const nlohmann::json& j, Power& inst) { inst.from_json(j); }
