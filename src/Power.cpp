#include <format>

#include <Power.hpp>
#include <PowersManager.hpp>
#include <HeroesHandler.hpp>
#include <MissionsHandler.hpp>
#include <Attribute.hpp>

using nlohmann::json;

// Power
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
bool Power::active() const { return unlocked && !disabled; }
bool Power::checkSlotRestriction(const EventData& args) {
	const std::vector<std::string>* assignedSlots = std::visit([](auto& d) -> const std::vector<std::string>* {
		if constexpr (requires { d.assignedSlots; }) { return d.assignedSlots; }
		else return nullptr; 
	}, args);

	if (assignedSlots) {
		const auto& slotsVec = *assignedSlots;
		int slotsCount = static_cast<int>(slotsVec.size());

		auto it = std::find(slotsVec.begin(), slotsVec.end(), hero);
		int slot = (it != slotsVec.end()) ? std::distance(slotsVec.begin(), it) : -1;

		if (slot != -1 && slotsCount > 0 && slotsCount <= slotRestriction.size()) {
			disabled = !slotRestriction[slotsCount - 1].contains(slot);
		}
	}

	return disabled;
}
// AttrBonusPower
std::set<Event> AttrBonusPower::getEventList() const {
	auto list = Power::getEventList();
	if (appliesTo == SELF) list.insert(Event::HeroCalcAttr);
	else list.merge(std::set<Event>{Event::AnyHeroCalcAttr, Event::MissionStart, Event::MissionSuccess, Event::MissionFailure});
	for (auto& [ev, _] : operations) list.insert(ev);
	return list;
}
void AttrBonusPower::onEvent(Event event, const EventData& args) {
	Power::onEvent(event, args);
	if (event == Event::HeroCalcAttr || event.is_any() || !operations.contains(event)) return;

	if (!checkSlotRestriction(args)) return;

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
void AttrBonusPower::onHeroCalcAttr(Event event, const HeroCalcAttrData& d) {
	Power::onHeroCalcAttr(event, d);
	(*d.attrs) += bonus;
}
void AttrBonusPower::onAnyHeroCalcAttr(Event event, const HeroCalcAttrData& d) {
	Power::onAnyHeroCalcAttr(event, d);

	auto& hh = HeroesHandler::inst();
	auto& mh = MissionsHandler::inst();

	if (hero.empty() || d.name.empty()) return;
	Hero& myHero = hh[hero];
	Hero& dHero = hh[d.name];

	std::string& myMissionName = myHero.mission;
	std::string& dMissionName = dHero.mission;
	if (myMissionName.empty() || dMissionName.empty() || myMissionName != dMissionName) return;
	Mission& mission = mh[myMissionName];

	int mySlot, dSlot;
	for (int i=0; i<mission.slots; i++) {
		if (mission.assignedSlots[i] == hero) mySlot = i;
		if (mission.assignedSlots[i] == d.name) dSlot = i;
	}

	if (applies(mySlot, dSlot)) (*d.attrs) += bonus;
}
void AttrBonusPower::onMissionStart(Event event, const MissionStartData& d) {
	Power::onMissionStart(event, d);
	onMission(*d.assignedSlots);
}
void AttrBonusPower::onMissionSuccess(Event event, const MissionSuccessData& d) {
	Power::onMissionSuccess(event, d);
	onMission(*d.assignedSlots);
}
void AttrBonusPower::onMissionFailure(Event event, const MissionFailureData& d) {
	Power::onMissionFailure(event, d);
	onMission(*d.assignedSlots);
}
void AttrBonusPower::onMission(const std::vector<std::string>& assignedSlots) {
	auto& hh = HeroesHandler::inst();
	auto it = std::find_if(BEGEND(assignedSlots), [&](const std::string& heroName) { return hero == heroName; });
	if (it != assignedSlots.end()) {
		int mySlot = it - assignedSlots.begin();
		for (int slot=0; slot < (int)assignedSlots.size(); slot++) {
			auto& heroName = assignedSlots[slot];
			if (!heroName.empty() && applies(mySlot, slot)) hh[heroName].needsAttrCalc = true;
		}
	}
}
bool AttrBonusPower::applies(int mySlot, int otherSlot) {
	switch (appliesTo) {
		case SELF:
			return otherSlot == mySlot;
		case OTHERS:
			return otherSlot != mySlot;
		case LEFT:
			return otherSlot == (mySlot-1);
		case RIGHT:
			return otherSlot == (mySlot+1);
		case ALL:
			return true;
		case ALL_LEFT:
			return otherSlot < mySlot;
		case ALL_RIGHT:
			return otherSlot > mySlot;
		default:
			return false;
	}
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

// Power
void Power::to_json(json& j) const {
	j = json{
		{"name", name},
		{"description", description},
		{"unlocked", unlocked},
		{"flight", flight},
		{"slotRestriction", slotRestriction},
	};
}
void Power::from_json(const json& j) {
	if (j.is_object()) {
		j.at("name").get_to(name);
		description = j.value("description", description);
		unlocked = j.value("unlocked", unlocked);
		flight = j.value("flight", flight);
		if (j.contains("slotRestriction")) {
			auto& js = j["slotRestriction"];
			if (js.is_array() && js.size() <= 4) {
				int i = 0;
				for (auto& jsv : js) {
					if (jsv.is_array()) slotRestriction[i] = jsv.get<std::set<int>>();
					else if (jsv.is_number_integer()) {
						int s = jsv.get<int>();
						slotRestriction[i] = { s };
					} else throw std::runtime_error("Invalid format for Power::slotRestriction: " + jsv.dump());
					i++;
				}
				while(i < 4) slotRestriction[i++].clear();
			} else if (js.is_number_integer()) {
				int s = js.get<int>();
				for (auto& v : slotRestriction) v = { s };
			} else throw std::runtime_error("Invalid format for Power::slotRestriction: " + js.dump());
		} else for (auto& v : slotRestriction) v = { 0, 1, 2, 3 };
	} else throw std::runtime_error("Invalid format for Power");
}
// AttrBonusPower
void AttrBonusPower::to_json(json& j) const {
	Power::to_json(j);
	WRITE(operations);
	WRITE(lowerLimit);
	WRITE(upperLimit);
	WRITE(appliesTo);
}
void AttrBonusPower::from_json(const json& j) {
	Power::from_json(j);
	READ(j, lowerLimit);
	READ(j, upperLimit);
	READ(j, appliesTo);
	operations.clear();
	const auto& jops = j.at("operations");
	for (auto& [eventName, _] : jops.items()) {
		const nlohmann::json& arr = jops.at(eventName);
		Event e = static_cast<Event>(eventName);
		operations[e] = arr.get<std::vector<AttrBonusPower::Operation>>();
	}
	if (j.contains("limit")) {
		if (j["limit"].is_array() && j["limit"].size() == 2) {
			lowerLimit = j["limit"][0].get<int>();
			upperLimit = j["limit"][1].get<int>();
		} else throw std::runtime_error("Invalid format for AttrBonusPower limit: " + j["limit"].dump());
	}
	std::cerr << json{*this}.dump(4) << std::endl;
}

// Partial Structs
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
