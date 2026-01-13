#include <algorithm>
#include <memory>
#include <iostream>

#include <EventHandler.hpp>
#include <Effect.hpp>
#include <Hero.hpp>

using nlohmann::json;

EventHandler::EventHandler() {}
EventHandler& EventHandler::inst() {
	static EventHandler singleton;
	return singleton;
}

template<typename T, typename CleanupFunc>
void processListenersCheck(Listeners<T>& container, Event event, EventData& args, const std::unordered_set<std::string>& targetHeroes, bool& result, CleanupFunc cleanup) {
	if (!result) return;
	
	auto it = container.find(event);
	if (it == container.end()) return;

	for (auto& weak : it->second) {
		auto shared = weak.lock();
		if (!shared) {
			cleanup(event, weak);
			continue;
		}
		if constexpr (requires { { shared->hero } -> std::convertible_to<std::string>; }) {
			if (!targetHeroes.empty() && !targetHeroes.contains(shared->hero)) continue;
		} else if constexpr (requires { { shared->hero->name } -> std::convertible_to<std::string>; }) {
			if (!targetHeroes.empty() && !targetHeroes.contains(shared->hero->name)) continue;
		} else {
			static_assert(sizeof(shared) == 0, "T must have a 'hero' string or a 'hero.name' string.");
		}
		if (!shared->onCheck(event, args)) {
			result = false;
			return;
		}
	}
}
bool EventHandler::check(Event event, EventData& args, const std::unordered_set<std::string>& targetHeroes) {
	updateListeners();
	bool result = true;

	auto cleanup = [this](Event e, auto&& weak) {  this->off(e, Listener{weak});  };

	#define CHECK(NAME) processListenersCheck(NAME##Listeners, event, args, targetHeroes, result, cleanup);
	LISTENER_TYPES(CHECK)
	#undef CHECK

	if (!result) return false;

	if (event.is_base() && !targetHeroes.empty()) return check(event.to_any(), args, {});
	return true;
}

template<typename T, typename CleanupFunc>
void processListenersCall(Listeners<T>& container, Event event, const EventData& args, const std::unordered_set<std::string>& targetHeroes, CleanupFunc cleanup) {
	auto it = container.find(event);
	if (it == container.end()) return;

	for (auto& weak : it->second) {
		auto shared = weak.lock();
		if (!shared) {
			cleanup(event, weak);
			continue;
		}
		if constexpr (requires { { shared->hero } -> std::convertible_to<std::string>; }) {
			if (!targetHeroes.empty() && !targetHeroes.contains(shared->hero)) continue;
		} else if constexpr (requires { { shared->hero->name } -> std::convertible_to<std::string>; }) {
			if (!targetHeroes.empty() && !targetHeroes.contains(shared->hero->name)) continue;
		} else {
			static_assert(sizeof(shared) == 0, "T must have a 'hero' string or a 'hero->name' string.");
		}
		shared->onEvent(event, args);
	}
}
void EventHandler::call(Event event, const EventData& args, const std::unordered_set<std::string>& targetHeroes) {
	updateListeners();

	auto cleanup = [this](Event e, auto&& weak) {  this->off(e, Listener{weak});  };

	#define AS_CALL(NAME) processListenersCall(NAME##Listeners, event, args, targetHeroes, cleanup);
	LISTENER_TYPES(AS_CALL)
	#undef AS_CALL

	if (event.is_base() && !targetHeroes.empty()) call(event.to_any(), args, {});
}

void EventHandler::on(Event event, Listener listener) {
	std::visit([this, event](auto&& arg){
		using T = std::decay_t<decltype(arg)>;
		#define AS_ON(NAME) if constexpr (std::is_same_v<T, std::weak_ptr<NAME>>) { if (!arg.expired()) { this->NAME##ToListen.emplace_back(event, arg); } }
		LISTENER_TYPES(AS_ON)
		#undef AS_ON
	}, listener);
}
void EventHandler::off(Event event, Listener listener) {
	std::visit([this, event](auto&& arg){
		using T = std::decay_t<decltype(arg)>;
		#define AS_OFF(NAME) if constexpr (std::is_same_v<T, std::weak_ptr<NAME>>) { this->NAME##ToUnlisten.emplace_back(event, arg); }
		LISTENER_TYPES(AS_OFF)
		#undef AS_OFF
	}, listener);
}

template<typename T>
void processUpdateListeners(Listeners<T>& listeners, toListeners<T>& toListen, toListeners<T>& toUnlisten) {
	for (auto& [event, listener] : toListen) listeners[event].insert(listener);
	for (auto& [event, listener] : toUnlisten) listeners[event].erase(listener);
	toListen.clear();
	toUnlisten.clear();
}
void EventHandler::updateListeners() {
	#define AS_UPDATE(NAME) processUpdateListeners(NAME##Listeners, NAME##ToListen, NAME##ToUnlisten);
	LISTENER_TYPES(AS_UPDATE)
	#undef AS_UPDATE
}
