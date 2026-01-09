#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include <Attribute.hpp>

#define EVENT_LIST(V)  \
	V(MISSION_START)   \
	V(MISSION_SUCCESS) \
	V(MISSION_FAILURE) \
	V(HERO_CALC_ATTR)

#define EVENT_DATA_LIST(V) \
	V(MissionStartData)    \
	V(MissionSuccessData)  \
	V(MissionFailureData)  \
	V(HeroCalcAttrData)    \
	V(GlobalData)

struct MissionStartData { std::string name; const std::vector<std::string>* assignedSlots; };
struct MissionSuccessData { std::string name; const std::vector<std::string>* assignedSlots; };
struct MissionFailureData { std::string name; const std::vector<std::string>* assignedSlots; };
struct HeroCalcAttrData { std::string name; Attribute attr{Attribute::COMBAT}; int* val; };
struct GlobalData {};

using EventData = std::variant<
	#define AS_VARIANT(TYPE) TYPE,
	EVENT_DATA_LIST(AS_VARIANT)
	#undef AS_VARIANT
std::monostate>;

class Event {
public:
	enum Type {
		#define AS_ENUM(NAME) NAME,
		EVENT_LIST(AS_ENUM)
		#undef AS_ENUM
		UNKNOWN = -1
	};
private:
	Type value;
public:
	Event(Type v) : value(v) {}
	Event(int i) : value(static_cast<Type>(i)) {}
	Event(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		static std::unordered_map<std::string, Type> toString {
			#define AS_MAP(NAME) {#NAME, NAME},
			EVENT_LIST(AS_MAP)
			#undef AS_MAP
		};
		value = toString.contains(s) ? toString[s] : UNKNOWN;
	}

	operator int() const { return static_cast<int>(value); }
	operator std::string() const {
		switch(value) {
			#define AS_STRING(NAME) case NAME: return #NAME;
			EVENT_LIST(AS_STRING)
			#undef AS_STRING
			default: return "UNKNOWN";
		}
	}

	static inline const std::vector<Type> ALL = {
		#define AS_VECTOR(NAME) NAME,
		EVENT_LIST(AS_VECTOR)
		#undef AS_VECTOR
	};

	static EventData CreateData(Type t) {
		switch(t) {
			case MISSION_START: return MissionStartData{};
			case MISSION_SUCCESS: return MissionSuccessData{};
			case MISSION_FAILURE: return MissionFailureData{};
			case HERO_CALC_ATTR: return HeroCalcAttrData{};
			default: return GlobalData{};
		}
	}
};

namespace std {
	template <>
	struct hash<Event> { std::size_t operator()(const Event& e) const { return std::hash<int>{}(static_cast<int>(e)); } };
}
