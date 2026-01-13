#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include <nlohmann/json.hpp>

#include <Effect.hpp>
#include <Event.hpp>

#define LISTENER_TYPES(V) \
	V(Effect)

#define AS_WEAK_PTR(NAME) std::weak_ptr<NAME>,
using Listener = std::variant< LISTENER_TYPES(AS_WEAK_PTR) std::monostate>;
#undef AS_WEAK_PTR
template<typename T>
using Listeners = std::map<Event, std::set<std::weak_ptr<T>, std::owner_less<std::weak_ptr<T>>>>;
template<typename T>
using toListeners = std::vector<std::pair<Event, std::weak_ptr<T>>>;


class EventHandler {
private:
	EventHandler();
	#define AS_MAP(NAME) \
		Listeners<NAME> NAME##Listeners; \
		toListeners<NAME> NAME##ToListen, NAME##ToUnlisten;
	LISTENER_TYPES(AS_MAP)
	#undef AS_MAP
	void updateListeners();
public:
	static EventHandler& inst();

	// If any listener returns false, the event returns false immediatelly, otherwise, returns true
	bool check(Event event, EventData& data, const std::unordered_set<std::string>& targetHeroes={});
	// Always calls all listeners
	void call(Event event, const EventData& data, const std::unordered_set<std::string>& targetHeroes={});

	// Variadic function to automatically construct the proper EventData type
	template <Event::Type T, typename... Args>
	void emit(const std::unordered_set<std::string>& targetHeroes, Args&&... args) {
		using DataType = typename Event::TypeToData<T>::Type;
		DataType d{ std::forward<Args>(args)... };
		call(T, d, targetHeroes);
	}

	void on(Event event, Listener listener);
	void off(Event event, Listener listener);
};
