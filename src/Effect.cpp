#include <format>
#include <string_view>

using namespace std::literals;

#include <Utils.hpp>
#include <Effect.hpp>
#include <EventHandler.hpp>
#include <HeroesHandler.hpp>
#include <MissionsHandler.hpp>
#include <Attribute.hpp>

using nlohmann::json;

// Effect
std::shared_ptr<Effect> Effect::effect_factory(const std::string& t) {
	auto type = Utils::toLower(t);
	if (type == "attrbonuseffect") return std::make_shared<AttrBonusEffect>();
	// else if (type == "aaa") return std::make_unique<aaa>();
	else throw std::runtime_error(std::format("Invalid power type '{}'", type));
}
std::shared_ptr<Effect> Effect::effect_factory(const json& data, Hero* h, Power* p) {
	const std::string& type = data.at("type").get<std::string>();
	auto effect = effect_factory(type);
	effect->hero = h;
	effect->power = p;
	try { effect->from_json(data); }
	catch (const std::exception& e) { throw std::runtime_error(std::format("Layout Error: Failed to deserialize effect of type '{}'. {}", type, e.what())); }

	auto& eh = EventHandler::inst();
	auto weak = effect->weak_from_this();
	for (Event ev : effect->getEventList()) eh.on(ev, Listener{weak});

	return effect;
}

std::set<Event> Effect::getEventList() const { return {}; }
bool Effect::active() const { return power->unlocked && !disabled; }
bool Effect::checkSlotRestriction(const EventData& args) {
	const std::vector<std::string>* assignedSlots = std::visit([](auto& d) -> const std::vector<std::string>* {
		if constexpr (requires { d.assignedSlots; }) { return d.assignedSlots; }
		else return nullptr; 
	}, args);

	if (assignedSlots) {
		const auto& slotsVec = *assignedSlots;
		size_t slotsCount = slotsVec.size();

		auto it = std::find(slotsVec.begin(), slotsVec.end(), hero->name);
		int slot = (it != slotsVec.end()) ? std::distance(slotsVec.begin(), it) : -1;

		if (slot != -1 && slotsCount > 0 && slotsCount <= slotRestriction.size()) {
			disabled = !slotRestriction[slotsCount - 1].contains(slot);
		}
	}

	return !disabled;
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
bool Effect::onCheck(Event event, const EventData& args) {
	if (!active()) return true;
	return std::visit(overloaded {
		#define GEN_VISIT(NAME, DATA) [this, event](const DATA& d) { \
			if (event == Event::NAME) return check##NAME(event, d);  \
			else return checkAny##NAME(event, d);                    \
		},
		BASE_EVENT_LIST(GEN_VISIT)
		#undef GEN_VISIT
		[](auto& /* ignore */) { return true; }
	}, args);
}
void Effect::onEvent(Event event, const EventData& args) {
	if (!power->unlocked) return;
	std::visit(overloaded {
		#define GEN_VISIT(NAME, DATA) [this, event](const DATA& d) { \
			if (event == Event::NAME) on##NAME(event, d);            \
			else onAny##NAME(event, d);                              \
		},
		BASE_EVENT_LIST(GEN_VISIT)
		#undef GEN_VISIT
		[](const auto& /* ignore */) {}
	}, args);
}

#ifndef __INTELLISENSE__
#define GEN_VIRTUALS(NAME, DATA) \
	bool Effect::check##NAME(Event, const DATA&) { return true; } \
	bool Effect::checkAny##NAME(Event, const DATA&) { return true; } \
	void Effect::on##NAME(Event, const DATA&) { /* Utils::println("	on{}: power {} of hero {}", #NAME, power->name, hero->name); */ } \
	void Effect::onAny##NAME(Event, const DATA&) { /* Utils::println("	onAny{}: power {} of hero {}", #NAME, power->name, hero->name); */ }
BASE_EVENT_LIST(GEN_VIRTUALS)
#undef GEN_VIRTUALS
#endif

// AttrBonusEffect
std::set<Event> AttrBonusEffect::getEventList() const {
	auto list = Effect::getEventList();
	if (appliesTo == SELF) list.insert(Event::HeroCalcAttr);
	else list.merge(std::set<Event>{Event::AnyHeroCalcAttr, Event::MissionStart, Event::MissionSuccess, Event::MissionFailure});
	for (auto& [ev, _] : operations) list.insert(ev);
	return list;
}
void AttrBonusEffect::onEvent(Event event, const EventData& args) {
	Effect::onEvent(event, args);
	if (event == Event::HeroCalcAttr || event.is_any() || !operations.contains(event)) return;
	if (!checkSlotRestriction(args)) return;

	auto& ops = operations[event];
	for (auto& [var, oper, value] : ops) {
		Attribute attr{Attribute::COMBAT};
		int val = 1;

		if (std::holds_alternative<Attribute>(var)) attr = std::get<Attribute>(var);
		if (std::holds_alternative<int>(value)) val = std::get<int>(value);

		if (std::holds_alternative<std::string>(var) || std::holds_alternative<std::string>(value)) {
			auto& mh = MissionsHandler::inst();
			auto mission = std::visit([&mh](auto& d) -> Utils::optRef<Mission> {
				if constexpr (requires { d.assignedSlots; d.name; }) { return mh[d.name]; }
				else return std::nullopt;
			}, args);

			AttrMap<int> heroAttrs, requiredAttrs;
			if (hero) heroAttrs = hero->attributes();
			if (mission) requiredAttrs = mission->get().requiredAttributes;

			auto getAttr = [&](std::string& s) -> AttrMap<int> {
				AttrMap<int> result;
				if (s.starts_with("hero"sv) && hero) {
					result = heroAttrs;
					s = s.substr(4);
				} else if (s.starts_with("mission"sv) && mission) {
					result = requiredAttrs;
					s = s.substr(7);
				} else if (s.starts_with("excess"sv) && hero && mission) {
					for (Attribute a : Attribute::Values) result[a] = std::max(0, heroAttrs[a] - requiredAttrs[a]);
					s = s.substr(6);
				} else if (s.starts_with("missing"sv) && hero && mission) {
					for (Attribute a : Attribute::Values) result[a] = std::max(0, requiredAttrs[a] - heroAttrs[a]);
					s = s.substr(7);
				} else throw std::runtime_error("Unknown or invalid operation source string '" + s + "' for event " + std::string(static_cast<std::string>(event)));
				return result;
			};

			if (std::holds_alternative<std::string>(var)) {
				std::string attrStr = std::get<std::string>(var);
				auto attrsMap = getAttr(attrStr);

				if (attrStr == "Lowest") {
					int minVal = std::numeric_limits<int>::max();
					std::set<Attribute> lowest;
					for (Attribute a : Attribute::Values) {
						if (attrsMap[a] < minVal) {
							minVal = attrsMap[a];
							lowest.clear();
							lowest.insert(a);
						} else if (attrsMap[a] == minVal) {
							lowest.insert(a);
						}
					}
					attr = Utils::random_element(lowest);
				} else if (attrStr == "Highest") {
					int maxVal = 0;
					std::set<Attribute> highest;
					for (Attribute a : Attribute::Values) {
						if (attrsMap[a] > maxVal) {
							maxVal = attrsMap[a];
							highest.clear();
							highest.insert(a);
						} else if (attrsMap[a] == maxVal) {
							highest.insert(a);
						}
					}
					attr = Utils::random_element(highest);
				} else if (attrStr == "Random") {
					attr = Attribute::Values[Utils::randInt(0, Attribute::COUNT - 1)];
				} else throw std::runtime_error("Unknown or invalid attribute type string '" + attrStr + "' for event " + std::string(static_cast<std::string>(event)));
			}

			if (std::holds_alternative<std::string>(value)) {
				std::string valStr = std::get<std::string>(value);
				auto attrsMap = getAttr(valStr);

				if (valStr == "Lowest") {
					int minVal = std::numeric_limits<int>::max();
					for (Attribute a : Attribute::Values) if (attrsMap[a] < minVal) minVal = attrsMap[a];
					val = minVal;
				} else if (valStr == "Highest") {
					int maxVal = 0;
					for (Attribute a : Attribute::Values) if (attrsMap[a] > maxVal) maxVal = attrsMap[a];
					val = maxVal;
				} else if (valStr == "Random") {
					attr = Attribute::Values[Utils::randInt(0, Attribute::COUNT - 1)];
				} else if (Attribute::isValid(valStr)) {
					Attribute vAttr = Attribute::fromString(valStr);
					val = attrsMap[vAttr];
				} else throw std::runtime_error("Unknown or invalid attribute type string '" + valStr + "' for event " + std::string(static_cast<std::string>(event)));
			}
		}

		switch (oper) {
			case Operator::PLUS:
				bonus[attr] += val;
				break;
			case Operator::MINUS:
				bonus[attr] -= val;
				break;
			case Operator::MULTIPLY:
				bonus[attr] *= val;
				break;
			case Operator::DIVIDE:
				bonus[attr] /= val;
				break;
			case Operator::ASSIGN:
				bonus[attr] = val;
				break;
		}
		bonus[attr] = std::clamp(bonus[attr], lowerLimit, upperLimit);
	}
	hero->needsAttrCalc = true;
}
void AttrBonusEffect::onHeroCalcAttr(Event event, const HeroCalcAttrData& d) {
	Effect::onHeroCalcAttr(event, d);
	(*d.attrs) += bonus;
}
void AttrBonusEffect::onAnyHeroCalcAttr(Event event, const HeroCalcAttrData& d) {
	Effect::onAnyHeroCalcAttr(event, d);

	auto& hh = HeroesHandler::inst();
	auto& mh = MissionsHandler::inst();

	if (d.name.empty()) return;
	Hero& myHero = *hero;
	Hero& dHero = hh[d.name];

	std::string& myMissionName = myHero.mission;
	std::string& dMissionName = dHero.mission;
	if (myMissionName.empty() || dMissionName.empty() || myMissionName != dMissionName) return;
	Mission& mission = mh[myMissionName];

	int mySlot=-1, dSlot=-1;
	for (int i=0; i<mission.slots; i++) {
		if (mission.assignedSlots[i] == myHero.name) mySlot = i;
		if (mission.assignedSlots[i] == d.name) dSlot = i;
	}

	if (applies(mySlot, dSlot)) (*d.attrs) += bonus;
}
void AttrBonusEffect::onMissionStart(Event event, const MissionStartData& d) {
	Effect::onMissionStart(event, d);
	onMission(*d.assignedSlots);
}
void AttrBonusEffect::onMissionSuccess(Event event, const MissionSuccessData& d) {
	Effect::onMissionSuccess(event, d);
	onMission(*d.assignedSlots);
}
void AttrBonusEffect::onMissionFailure(Event event, const MissionFailureData& d) {
	Effect::onMissionFailure(event, d);
	onMission(*d.assignedSlots);
}
void AttrBonusEffect::onMission(const std::vector<std::string>& assignedSlots) {
	auto& hh = HeroesHandler::inst();
	auto it = std::find_if(BEGEND(assignedSlots), [&](const std::string& heroName) { return hero->name == heroName; });
	if (it != assignedSlots.end()) {
		int mySlot = it - assignedSlots.begin();
		for (int slot=0; slot < (int)assignedSlots.size(); slot++) {
			auto& heroName = assignedSlots[slot];
			if (!heroName.empty() && applies(mySlot, slot)) hh[heroName].needsAttrCalc = true;
		}
	}
}
bool AttrBonusEffect::applies(int mySlot, int otherSlot) {
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
							else throw std::invalid_argument("Effect '" + std::string(#var) + "' cannot be empty"); \
						}
#define READ2(j, var, key)	var = j.value(#key, var)
#define READREQ2(j, var, key) { \
							if (j.contains(#key)) j.at(#key).get_to(var); \
							else throw std::invalid_argument("Effect '" + std::string(#key) + "' cannot be empty"); \
						}
#define WRITE(var)			j[#var] = var;
#define WRITE2(var, key)	j[#key] = var;

// Effect
void Effect::to_json(json& j) const {
	j = json{
		{"slotRestriction", slotRestriction},
	};
}
void Effect::from_json(const json& j) {
	if (j.is_object()) {
		if (j.contains("slotRestriction")) {
			auto& js = j["slotRestriction"];
			if (js.is_array() && js.size() <= 4) {
				int i = 0;
				for (auto& jsv : js) {
					if (jsv.is_array()) slotRestriction[i] = jsv.get<std::set<int>>();
					else if (jsv.is_number_integer()) {
						int s = jsv.get<int>();
						slotRestriction[i] = { s };
					} else throw std::runtime_error("Invalid format for Effect::slotRestriction: " + jsv.dump());
					i++;
				}
				while(i < 4) slotRestriction[i++].clear();
			} else if (js.is_number_integer()) {
				int s = js.get<int>();
				for (auto& v : slotRestriction) v = { s };
			} else throw std::runtime_error("Invalid format for Effect::slotRestriction: " + js.dump());
		} else for (auto& v : slotRestriction) v = { 0, 1, 2, 3 };
	} else throw std::runtime_error("Invalid format for Effect");
}
// AttrBonusEffect
void AttrBonusEffect::to_json(json& j) const {
	Effect::to_json(j);
	WRITE(operations);
	WRITE(lowerLimit);
	WRITE(upperLimit);
	WRITE(appliesTo);
}
void AttrBonusEffect::from_json(const json& j) {
	Effect::from_json(j);
	READ(j, lowerLimit);
	READ(j, upperLimit);
	READ(j, appliesTo);
	operations.clear();
	const auto& jops = j.at("operations");
	for (auto& [eventName, _] : jops.items()) {
		const nlohmann::json& arr = jops.at(eventName);
		Event e = static_cast<Event>(eventName);
		operations[e] = arr.get<std::vector<AttrBonusEffect::Operation>>();
		if (j.contains("limit")) {
			if (j["limit"].is_array() && j["limit"].size() == 2) {
				lowerLimit = j["limit"][0].get<int>();
				upperLimit = j["limit"][1].get<int>();
			} else throw std::runtime_error("Invalid format for AttrBonusEffect limit: " + j["limit"].dump());
		}
	}
}

// Partial Structs
void to_json(json& j, const AttrBonusEffect::Operation& inst) {
	j = json { { "operator", inst.oper } };
	std::visit([&j](auto& attr) { j["attribute"] = attr; }, inst.attribute);
	std::visit([&j](auto& val) { j["value"] = val; }, inst.value);
}
void from_json(const json& j, AttrBonusEffect::Operation& inst) {
	std::string attr;
	json value;
	if (j.is_object()) {
		j.at("attribute").get_to(attr);
		j.at("operator").get_to(inst.oper);
		value = j.at("value");
	} else if (j.is_array() && (int)j.size() == 3) {
		j[0].get_to(attr);
		j[1].get_to(inst.oper);
		value = j[2];
	} else throw std::runtime_error("Invalid format for AttrBonusEffect::Operation");

	if (Attribute::isValid(attr)) inst.attribute = Attribute(attr);
	else inst.attribute = attr;

	if (value.is_number_integer()) inst.value = value.get<int>();
	else if (value.is_string()) inst.value = value.get<std::string>();
	else throw std::runtime_error("Invalid format for AttrBonusEffect::Operation value");
}
