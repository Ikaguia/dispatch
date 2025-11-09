#pragma once

#include <string>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include <Utils.hpp>

class Attribute {
public:
	enum Value {
		COMBAT,
		VIGOR,
		MOBILITY,
		CHARISMA,
		INTELLIGENCE,
		COUNT
	};

	constexpr Attribute(Value v) : value(v) {}
	constexpr operator Value() const { return value; }
	constexpr bool operator==(const Attribute& rhs) const noexcept { return value == rhs.value; }

	constexpr std::string_view toString() const {
		switch (value) {
			case COMBAT: return "Combat";
			case VIGOR: return "Vigor";
			case MOBILITY: return "Mobility";
			case CHARISMA: return "Charisma";
			case INTELLIGENCE: return "Intelligence";
			default: return "Unknown";
		}
	}

	static constexpr Attribute fromString(std::string_view s) {
		if (Utils::equals(s, "com", "combat")) return COMBAT;
		if (Utils::equals(s, "vig", "vigor")) return VIGOR;
		if (Utils::equals(s, "mob", "mobility")) return MOBILITY;
		if (Utils::equals(s, "cha", "charisma")) return CHARISMA;
		if (Utils::equals(s, "int", "intelligence")) return INTELLIGENCE;
		throw std::invalid_argument("Unknown attribute: " + std::string(s));
	}

private:
	Value value;
};

struct AttrHash {
	constexpr size_t operator()(const Attribute &a) const noexcept {
		return static_cast<size_t>((Attribute::Value)a);
	}
};

template<typename ValueType>
using AttrMap = std::unordered_map<Attribute, ValueType, AttrHash>;
