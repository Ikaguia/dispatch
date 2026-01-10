#include <algorithm>
#include <memory>
#include <iostream>

#include <PowersManager.hpp>

using nlohmann::json;


PowersManager::PowersManager() {
	for (auto ev : Event::ALL) {
		listeners[ev] = {};
		toListen[ev] = {};
		toUnlisten[ev] = {};
	}
}
PowersManager& PowersManager::inst() {
	static PowersManager singleton;
	return singleton;
}

void PowersManager::load(const json& data, const std::string& key) {
	try {
		auto power = Power::power_factory(data);
		for (Event ev : power->getEventList()) on(ev, key);
		powers[key] = std::move(power);
	} catch (std::exception& e) {
		std::cerr << "Key: " << key << ", data: " << data.dump(4) << std::endl;
		std::cerr << e.what() << std::endl;
		throw e;
	}
}
void PowersManager::unload(const std::string& key) {
	powers.erase(key);
	for (auto& [ev, _] : listeners) toUnlisten[ev].push_back(key);
}
void PowersManager::clear() { powers.clear(); }

bool PowersManager::has(const std::string& key) const { return powers.contains(key); }

const Power& PowersManager::operator[](const std::string& key) const { return *(powers.at(key)); }
Power& PowersManager::operator[](const std::string& key) { return *(powers.at(key)); }

bool PowersManager::check(Event event, EventData& args, const std::unordered_set<std::string>& heroes) {
	updateListeners();
	bool result = true;
	auto& lis = listeners[event];
	for (auto& key : lis) {
		auto& power = (*this)[key];
		if (!heroes.empty() && !heroes.contains(power.hero)) continue;
		if (!power.onCheck(event, args)) {
			result = false;
			break;
		}
	}
	if (event.is_base() && !heroes.empty()) result = result && check(event.to_any(), args, {});
	return result;
}
void PowersManager::call(Event event, EventData& args, const std::unordered_set<std::string>& heroes) {
	updateListeners();
	auto& lis = listeners[event];
	for (auto& key : lis) {
		auto& power = (*this)[key];
		if (!heroes.empty() && !heroes.contains(power.hero)) continue;
		power.onEvent(event, args);
	}
	if (event.is_base() && !heroes.empty()) call(event.to_any(), args, {});
}

void PowersManager::on(Event event, const std::string& key) { toListen[event].push_back(key); }
void PowersManager::off(Event event, const std::string& key) { toUnlisten[event].push_back(key); }
void PowersManager::updateListeners() {
	for (auto& [event, lis] : listeners) {
		auto& listen = toListen[event];
		for (auto& key : listen) lis.insert(key);
		listen.clear();

		auto& unlisten = toUnlisten[event];
		for (auto& key : unlisten) lis.erase(key);
		unlisten.clear();
	}
}
