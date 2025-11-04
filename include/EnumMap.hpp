#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <stdexcept>

template<typename Enum, typename Value, Enum MaxEnum>
class EnumMap {
public:
	static constexpr size_t size = static_cast<size_t>(MaxEnum);
private:
	std::array<Value, size> data;
public:

	EnumMap() = default;
	EnumMap(std::initializer_list<Value> init) {
		if (init.size() != size) throw std::invalid_argument("EnumMap initializer_list must match size");
		std::copy(init.begin(), init.end(), data.begin());
	}

	constexpr Value& operator[](Enum key) {
		return data[static_cast<size_t>(key)];
	}
	constexpr const Value& operator[](Enum key) const {
		return data[static_cast<size_t>(key)];
	}

	// --- Iterator support ---
	struct iterator {
		size_t index;
		EnumMap* map;

		constexpr iterator& operator++() { ++index; return *this; }
		constexpr bool operator!=(const iterator& other) const { return index != other.index; }

		constexpr auto operator*() const {
			return std::pair<Enum, Value&>{
				static_cast<Enum>(index),
				map->data[index]
			};
		}
	};

	struct const_iterator {
		size_t index;
		const EnumMap* map;

		constexpr const_iterator& operator++() { ++index; return *this; }
		constexpr bool operator!=(const const_iterator& other) const { return index != other.index; }

		constexpr auto operator*() const {
			return std::pair<Enum, const Value&>{
				static_cast<Enum>(index),
				map->data[index]
			};
		}
	};

	constexpr iterator begin() { return {0, this}; }
	constexpr iterator end() { return {size, this}; }

	constexpr const_iterator begin() const { return {0, this}; }
	constexpr const_iterator end() const { return {size, this}; }
	constexpr const_iterator cbegin() const { return {0, this}; }
	constexpr const_iterator cend() const { return {size, this}; }
};
