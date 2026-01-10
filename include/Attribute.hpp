#pragma once

#include <string>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>

namespace AttributeUtils {
	inline constexpr char toLower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c; }
	inline constexpr bool equals(std::string_view a, std::string_view b) {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i) if (toLower(a[i]) != toLower(b[i])) return false;
		return true;
	}
	template<typename... Args>
	inline constexpr bool equals(std::string_view a, std::string_view b, Args... args) { return ((equals(a, b) || ... || equals(a, args))); }
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
	inline static constexpr Value Values[COUNT] = { COMBAT, VIGOR, MOBILITY, CHARISMA, INTELLIGENCE };
	inline static constexpr std::string_view Icons[COUNT] = {
		"⚔", // COMBAT
		"🛡", // VIGOR
		"🏃", // MOBILITY
		"💬", // CHARISMA
		"🧠"  // INTELLIGENCE
	};
	
	constexpr Attribute(Value v) : value(v) {}
	constexpr Attribute(int i) : value(Values[i]) {}
	constexpr Attribute(std::string s) : value(fromString(s)) {}

	constexpr operator Value() const { return value; }
	constexpr operator std::string_view() const { return toString(); }
	constexpr operator std::string() const { return std::string{toString()}; }

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
	constexpr std::string_view toIcon() const { return Icons[value]; }

	static constexpr Attribute fromString(std::string_view s) {
		if (AttributeUtils::equals(s, "com", "combat")) return COMBAT;
		if (AttributeUtils::equals(s, "vig", "vigor")) return VIGOR;
		if (AttributeUtils::equals(s, "mob", "mobility")) return MOBILITY;
		if (AttributeUtils::equals(s, "cha", "charisma")) return CHARISMA;
		if (AttributeUtils::equals(s, "int", "intelligence")) return INTELLIGENCE;
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

template <typename ValueType>
class AttrMap : public std::unordered_map<Attribute::Value, ValueType, AttrHash> {
public:
	AttrMap() { for (auto attr : Attribute::Values) (*this)[attr] = ValueType{}; }

	ValueType& operator[](int idx) {
		if (idx < 0 || idx >= Attribute::COUNT) throw std::out_of_range("Invalid Attribute index");
		return std::unordered_map<Attribute::Value, ValueType, AttrHash>::operator[](Attribute::Values[idx]);
	}

	const ValueType& operator[](int idx) const {
		if (idx < 0 || idx >= Attribute::COUNT) throw std::out_of_range("Invalid Attribute index");
		return this->at(Attribute::Values[idx]);
	}

	AttrMap<ValueType> operator+(const AttrMap<ValueType>& other) const {
		AttrMap<ValueType> result = *this;
		result += other;
		return result;
	}
	AttrMap<ValueType>& operator+=(const AttrMap<ValueType>& other) {
		for (Attribute attr : Attribute::Values) (*this)[attr] += other[attr];
		return *this;
	}

	AttrMap<ValueType> operator-(const AttrMap<ValueType>& other) const {
		AttrMap<ValueType> result = *this;
		result -= other;
		return result;
	}
	AttrMap<ValueType>& operator-=(const AttrMap<ValueType>& other) {
		for (Attribute attr : Attribute::Values) (*this)[attr] -= other[attr];
		return *this;
	}
};

namespace Utils { std::string toLower(std::string); }

namespace nlohmann {
	NLOHMANN_JSON_SERIALIZE_ENUM( Attribute::Value, {
		{ Attribute::Value::COMBAT, "COMBAT" },
		{ Attribute::Value::VIGOR, "VIGOR" },
		{ Attribute::Value::MOBILITY, "MOBILITY" },
		{ Attribute::Value::CHARISMA, "CHARISMA" },
		{ Attribute::Value::INTELLIGENCE, "INTELLIGENCE" },
	});
	inline void to_json(nlohmann::json& j, const Attribute& inst) { j = static_cast<Attribute::Value>(inst); }
	inline void from_json(const nlohmann::json& j, Attribute& inst) { inst = j.get<Attribute::Value>(); }


	template <typename ValueType>
	struct adl_serializer<AttrMap<ValueType>> {
		static void to_json(json& j, const AttrMap<ValueType>& attrs) {
			j = json{};
			for (Attribute attr : Attribute::Values) {
				std::string key = Utils::toLower((std::string)attr.toString());
				j[key] = attrs[attr];
			}
		}
		static void from_json(const json& j, AttrMap<ValueType>& attrs) {
			if (j.is_object()) {
				for (Attribute attr : Attribute::Values) {
					std::string key = Utils::toLower((std::string)attr.toString());
					attrs[attr] = j.at(key).get<ValueType>();
				}
			} else if (j.is_array()) {
				if (j.size() != Attribute::COUNT) throw std::runtime_error("Invalid number of elements for AttrMap");
				for (int i = 0; i < Attribute::COUNT; i++) attrs[Attribute::Values[i]] = j[i].get<ValueType>();
			} else throw std::runtime_error("Invalid format for AttrMap");
		}
	};
}
