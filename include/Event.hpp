#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include <nlohmann/json.hpp>

#include <Attribute.hpp>

#define BASE_EVENT_LIST(V) \
    V(MissionStart,   MissionStartData) \
    V(MissionSuccess, MissionSuccessData) \
    V(MissionFailure, MissionFailureData) \
    V(HeroCalcAttr,  HeroCalcAttrData)

struct MissionStartData { std::string name; const std::vector<std::string>* assignedSlots; };
struct MissionSuccessData { std::string name; const std::vector<std::string>* assignedSlots; };
struct MissionFailureData { std::string name; const std::vector<std::string>* assignedSlots; };
struct HeroCalcAttrData { std::string name; Attribute attr{Attribute::COMBAT}; int* val; };
struct GlobalData {};

using EventData = std::variant<
	#define AS_VARIANT(NAME, DATA) DATA,
	BASE_EVENT_LIST(AS_VARIANT)
	#undef AS_VARIANT
GlobalData, std::monostate>;

class Event {
public:
	enum Type {
		BASE_START = 0,
		#define AS_ENUM(NAME, DATA) NAME,
		BASE_EVENT_LIST(AS_ENUM)
		#undef AS_ENUM
		ANY_START,
		#define AS_ANY_ENUM(NAME, DATA) ANY_##NAME,
		BASE_EVENT_LIST(AS_ANY_ENUM)
		#undef AS_ANY_ENUM
		UNKNOWN = -1
	};
private:
	Type value;
public:
	Event() : value(UNKNOWN) {}
	Event(Type v) : value(v) {}
	Event(int i) : value(static_cast<Type>(i)) {}
	Event(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		static std::unordered_map<std::string, Type> toString {
			#define AS_MAP(NAME, DATA) {#NAME, NAME},
			BASE_EVENT_LIST(AS_MAP)
			#undef AS_MAP

			#define AS_ANY_MAP(NAME, DATA) {"ANY_"#NAME, ANY_##NAME},
			BASE_EVENT_LIST(AS_ANY_MAP)
			#undef AS_ANY_MAP
		};
		value = toString.contains(s) ? toString[s] : UNKNOWN;
	}

	operator int() const { return static_cast<int>(value); }
	operator std::string() const {
		switch(value) {
			#define AS_STRING(NAME, DATA) case NAME: return #NAME;
			BASE_EVENT_LIST(AS_STRING)
			#undef AS_STRING
			#define AS_ANY_STRING(NAME, DATA) case ANY_##NAME: return "ANY_" #NAME;
			BASE_EVENT_LIST(AS_ANY_STRING)
			#undef AS_ANY_STRING
			default: return "UNKNOWN";
		}
	}

	#define AS_VECTOR(NAME, DATA) NAME,
	#define AS_ANY_VECTOR(NAME, DATA) ANY_##NAME,
	static inline const std::vector<Type> ALL = {
		BASE_EVENT_LIST(AS_VECTOR)
		BASE_EVENT_LIST(AS_ANY_VECTOR)
	}, ALL_BASE = {
		BASE_EVENT_LIST(AS_VECTOR)
	}, ALL_ANY = {
		BASE_EVENT_LIST(AS_ANY_VECTOR)
	};
	#undef AS_VECTOR
	#undef AS_ANY_VECTOR

	bool is_base() const { return value > BASE_START && value < ANY_START; }
	bool is_any() const { return value > ANY_START; }

	Event to_base() const {
		if (is_base()) return value;
		if (is_any()) return value - ANY_START + BASE_START;
		throw std::runtime_error("Invalid Event::to_base conversion");
	}

	Event to_any() const {
		if (is_any()) return value;
		if (is_base()) return value + ANY_START - BASE_START;
		throw std::runtime_error("Invalid Event::to_any conversion");
	}

	static EventData CreateData(Type t) {
		switch(t) {
			#define AS_DATA(NAME, DATA) \
				case NAME:              \
				/* fallthrough */       \
				case ANY_##NAME:        \
				return DATA{};
			BASE_EVENT_LIST(AS_DATA)
			#undef AS_DATA

			default: return GlobalData{};
		}
	}
};

namespace std {
	template <>
	struct hash<Event> { std::size_t operator()(const Event& e) const { return std::hash<int>{}(static_cast<int>(e)); } };
}

namespace nlohmann {
	inline void to_json(nlohmann::json& j, const Event& inst) { j = static_cast<std::string>(inst); }
	inline void from_json(const nlohmann::json& j, Event& inst) { inst = j.get<std::string>(); }
}
