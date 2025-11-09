#pragma once

#include <utility>
#include <cstddef>
#include <iterator>
#include <random>

namespace Utils {
	std::string toUpper(std::string str);

	int randInt(int low, int high);

	namespace Detail {
		constexpr char toLower(char c) {
			return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
		}
	}

	constexpr bool equals(std::string_view a, std::string_view b) {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); ++i) if (Detail::toLower(a[i]) != Detail::toLower(b[i])) return false;
		return true;
	}
	template<typename... Args>
	constexpr bool equals(std::string_view a, std::string_view b, Args... args) { return equals(a, b) && equals(a, args...); }

	template <typename T>
	T clamp(T val, T mn, T mx) { return val < mn ? mn : val > mx ? mx : val; }

	template <typename T>
	constexpr auto enumerate(T&& iterable) {
		struct iterator {
			size_t i;
			decltype(std::begin(iterable)) iter;

			bool operator!=(const iterator& other) const { return iter != other.iter; }
			void operator++() { ++i; ++iter; }
			auto operator*() const { return std::tie(i, *iter); }
		};

		struct iterable_wrapper {
			T iterable;
			auto begin() { return iterator{0, std::begin(iterable)}; }
			auto end() { return iterator{0, std::end(iterable)}; }
		};

		return iterable_wrapper{std::forward<T>(iterable)};
	}

	template <typename Set>
	auto random_element(const Set& s) -> const typename Set::value_type& {
		static std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<size_t> dist(0, s.size() - 1);
		return *std::next(s.begin(), dist(rng));
	}
}
