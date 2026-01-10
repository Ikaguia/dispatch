#include <format>

#include <Power.hpp>
#include <PowersManager.hpp>
#include <HeroesHandler.hpp>
#include <Attribute.hpp>

using nlohmann::json;

std::unique_ptr<Power> Power::power_factory(const std::string& t) {
	auto type = Utils::toLower(t);
	if (type == "power") return std::make_unique<Power>();
	if (type == "attrbonuspower") return std::make_unique<AttrBonusPower>();
	// else if (type == "aaa") return std::make_unique<aaa>();
	else throw std::runtime_error(std::format("Invalid power type '{}'", type));
}
std::unique_ptr<Power> Power::power_factory(const json& data) {
	const std::string& name = data.at("name");
	// const std::string& type = data.at("type");
	const std::string type = data.value("type", "power");
	auto power = power_factory(type);
	try { power->from_json(data); }
	catch (const std::exception& e) { throw std::runtime_error(std::format("Layout Error: Failed to deserialize power '{}' of type '{}'. {}", name, type, e.what())); }
	return power;
}

std::set<Event> Power::getEventList() const { return {}; }

std::set<Event> AttrBonusPower::getEventList() const {
	auto list = Power::getEventList();
	list.insert(Event::HERO_CALC_ATTR);
	for (auto& [ev, _] : operations) list.insert(ev);
	return list;
}
void AttrBonusPower::onEvent(Event event, EventData& args) {
	Power::onEvent(event, args);
	if (event == Event::HERO_CALC_ATTR) return;

	auto& ops = operations[event];
	for (auto& [attr, oper, value] : ops) {
		switch (oper) {
			case Operator::PLUS:
				bonus[attr] += value;
				break;
			case Operator::MINUS:
				bonus[attr] -= value;
				break;
			case Operator::MULTIPLY:
				bonus[attr] *= value;
				break;
			case Operator::DIVIDE:
				bonus[attr] /= value;
				break;
			case Operator::ASSIGN:
				bonus[attr] = value;
				break;
		}
		bonus[attr] = std::clamp(bonus[attr], lowerLimit, upperLimit);
	}
	HeroesHandler::inst()[hero].needsAttrCalc = true;
}
bool AttrBonusPower::preHeroCalcAttrData(HeroCalcAttrData&) { return true; }
void AttrBonusPower::onHeroCalcAttrData(HeroCalcAttrData& d) {
	Power::onHeroCalcAttrData(d);
	(*d.val) += bonus[d.attr];
}


// JSON Serialization Macros
#define READ(j, var)		var = j.value(#var, var)
#define READREQ(j, var) { \
							if (j.contains(#var)) j.at(#var).get_to(var); \
							else throw std::invalid_argument("Hero '" + std::string(#var) + "' cannot be empty"); \
						}
#define READ2(j, var, key)	var = j.value(#key, var)
#define READREQ2(j, var, key) { \
							if (j.contains(#key)) j.at(#key).get_to(var); \
							else throw std::invalid_argument("Hero '" + std::string(#key) + "' cannot be empty"); \
						}
#define WRITE(var)			j[#var] = var;
#define WRITE2(var, key)	j[#key] = var;

void Power::to_json(json& j) const {
	j = json{
		{"name", name},
		{"description", description},
		{"unlocked", unlocked},
		{"flight", flight},
	};
}
void Power::from_json(const json& j) {
	if (j.is_object()) {
		j.at("name").get_to(name);
		description = j.value("description", description);
		unlocked = j.value("unlocked", unlocked);
		flight = j.value("flight", flight);
	} else throw std::runtime_error("Invalid format for Power");
}

void AttrBonusPower::to_json(json& j) const {
	Power::to_json(j);
	j["operations"] = operations;
	j["lowerLimit"] = lowerLimit;
	j["upperLimit"] = upperLimit;
}
void AttrBonusPower::from_json(const json& j) {
	Power::from_json(j);
	operations.clear();
	const auto& jops = j.at("operations");
	for (auto& [eventName, _] : jops.items()) {
		const nlohmann::json& arr = jops.at(eventName);
		Event e = static_cast<Event>(eventName);
		operations[e] = arr.get<std::vector<AttrBonusPower::Operation>>();
	}
	lowerLimit = j.value("lowerLimit", lowerLimit);
	upperLimit = j.value("upperLimit", upperLimit);
	if (j.contains("limit")) {
		if (j["limit"].is_array() && j["limit"].size() == 2) {
			lowerLimit = j["limit"][0].get<int>();
			upperLimit = j["limit"][1].get<int>();
		} else throw std::runtime_error("Invalid format for AttrBonusPower limit: " + j["limit"].dump());
	}
}

void to_json(json& j, const AttrBonusPower::Operation& inst) {
	j = json {
		{ "attribute", inst.attribute },
		{ "operator", inst.oper },
		{ "value", inst.value },
	};
}
void from_json(const json& j, AttrBonusPower::Operation& inst) {
	if (j.is_object()) {
		j.at("attribute").get_to(inst.attribute);
		j.at("operator").get_to(inst.oper);
		j.at("value").get_to(inst.value);
	} else if (j.is_array() && (int)j.size() == 3) {
		j[0].get_to(inst.attribute);
		j[1].get_to(inst.oper);
		j[2].get_to(inst.value);
	} else throw std::runtime_error("Invalid format for AttrBonusPower::Operation");
}
