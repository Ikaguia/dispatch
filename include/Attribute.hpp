#pragma once

#include <string>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace {
	constexpr char toLower(char c) {
		return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
	}

	constexpr bool equals(std::string_view a, std::string_view b) {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i) if (toLower(a[i]) != toLower(b[i])) return false;
		return true;
	}
	template<typename... Args>
	constexpr bool equals(std::string_view a, std::string_view b, Args... args) { return equals(a, b) && equals(a, args...); }
};


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
		if (equals(s, "com", "combat")) return COMBAT;
		if (equals(s, "vig", "vigor")) return VIGOR;
		if (equals(s, "mob", "mobility")) return MOBILITY;
		if (equals(s, "cha", "charisma")) return CHARISMA;
		if (equals(s, "int", "intelligence")) return INTELLIGENCE;
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
