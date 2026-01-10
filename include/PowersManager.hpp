#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <Power.hpp>
#include <Event.hpp>

class PowersManager {
private:
	PowersManager();
	std::unordered_map<std::string, std::unique_ptr<Power>> powers;
	std::map<Event, std::unordered_set<std::string>> listeners;
	std::map<Event, std::vector<std::string>> toListen, toUnlisten;
	void updateListeners();
public:
	static PowersManager& inst();

	void load(const nlohmann::json& data, const std::string& key);
	void unload(const std::string& key);
	void clear();

	bool has(const std::string& key) const;

	const Power& operator[](const std::string& key) const;
	Power& operator[](const std::string& key);

	// If any listener returns false, the event returns false immediatelly, otherwise, returns true
	bool call(Event event, EventData& data);
	// Always calls all listeners
	void callAll(Event event, EventData& data);

	void on(Event event, const std::string& key);
	void off(Event event, const std::string& key);
};
