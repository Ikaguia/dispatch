#pragma once

#include <string>
#include <map>
#include <set>
#include <iostream>
#include <memory>

#include <nlohmann/json.hpp>

class Mission;

#include <raylib-cpp.hpp>
#include <Attribute.hpp>
#include <Hero.hpp>
#include <UI.hpp>

class Disruption {
public:
	struct Option {
		std::string name, hero, attribute, successMessage="SUCCESS!", failureMessage="FAILURE!";
		int value;
		enum Type { HERO, ATTRIBUTE } type;
		bool disabled = false;

		static void to_json(nlohmann::json& j, const Option& option);
		static void from_json(const nlohmann::json& j, Option& option);
	};
	std::vector<Option> options;
	std::string description;
	float timeout, elapsedTime;
	int selected_option = -1;
	std::vector<raylib::Rectangle> optionButtons;

	static void to_json(nlohmann::json& j, const Disruption& disruption);
	static void from_json(const nlohmann::json& j, Disruption& disruption);
};

class Mission : public std::enable_shared_from_this<Mission> {
private:
	raylib::Rectangle btnCancel, btnStart;
public:
	enum Status {
		PENDING,
		SELECTED,
		TRAVELLING,
		PROGRESS,
		AWAITING_REVIEW,
		REVIEWING_SUCESS,
		REVIEWING_FAILURE,
		DONE,
		MISSED,
		DISRUPTION,
		DISRUPTION_MENU
	} status{PENDING};

	std::string name, type, caller, description, failureMsg="MISSION FAILED", failureMission, successMsg="MISSION COMPLETED", successMission;
	std::vector<std::string> requirements;
	std::vector<Disruption> disruptions;
	raylib::Vector2 position{0.0f, 0.0f};
	AttrMap<int> requiredAttributes{};
	int slots, difficulty=1, curDisruption=-1;
	float failureTime=60.0f, missionDuration=20.0f, failureMissionTime=0.0f, successMissionTime=0.0f, timeElapsed=0.0f;
	bool dangerous=false, triggered=false, disrupted=false;
	std::set<std::string> assignedHeroes;
	std::vector<std::string> assignedSlots;

	Mission(const std::string& name, const std::string& type, const std::string& caller, const std::string& description, const std::string& failureMsg, const std::string& failureMission, const std::string& successMsg, const std::string& successMission, const std::vector<std::string>& requirements, raylib::Vector2 pos, const std::map<std::string,int> &attr, int slots, int difficulty, float failureTime, float missionDuration, float failureMissionTimeool, float successMissionTime, bool dangerous);
	Mission(const nlohmann::json& data);
	Mission(const Mission&) = delete;
	Mission& operator=(const Mission&) = delete;

	void validate() const;

	void toggleHero(const std::string& name);
	void assignHero(const std::string& name);
	void unassignHero(const std::string& name);

	void changeStatus(Status newStatus);
	void update(float deltaTime);

	void renderUI(bool full=false);
	void handleInput();

	void setupLayout(Dispatch::UI::Layout& layout);
	void updateLayout(Dispatch::UI::Layout& layout, const std::string& changed);

	AttrMap<int> getTotalAttributes() const;
	int getTotalAttribute(Attribute attr) const;
	int getSuccessChance() const;
	bool isSuccessful() const;
	bool isDisruptionSuccessful() const;
	bool isMenuOpen() const;

	static std::string statusToString(Status st);

	static void to_json(nlohmann::json& j, const Mission& mission);
	static void from_json(const nlohmann::json& j, Mission& mission);
};

namespace nlohmann {
	template <>
	struct adl_serializer<Mission> {
		static void to_json(json& j, const Mission& mission) { Mission::to_json(j, mission); }
		static void from_json(const json& j, Mission& mission) { Mission::from_json(j, mission); }
	};

	NLOHMANN_JSON_SERIALIZE_ENUM( Mission::Status, {
		{ Mission::Status::PENDING, "pending" },
		{ Mission::Status::SELECTED, "selected" },
		{ Mission::Status::TRAVELLING, "travelling" },
		{ Mission::Status::PROGRESS, "progress" },
		{ Mission::Status::AWAITING_REVIEW, "awaiting_review" },
		{ Mission::Status::REVIEWING_SUCESS, "reviewing_sucess" },
		{ Mission::Status::REVIEWING_FAILURE, "reviewing_failure" },
		{ Mission::Status::DONE, "done" },
		{ Mission::Status::MISSED, "missed" },
		{ Mission::Status::DISRUPTION, "disruption" },
		{ Mission::Status::DISRUPTION_MENU, "disruption_menu" },
	});

	template <>
	struct adl_serializer<Disruption> {
		static void to_json(json& j, const Disruption& disruption) { Disruption::to_json(j, disruption); }
		static void from_json(const json& j, Disruption& disruption) { Disruption::from_json(j, disruption); }
	};

	template <>
	struct adl_serializer<Disruption::Option> {
		static void to_json(json& j, const Disruption::Option& option) { Disruption::Option::to_json(j, option); }
		static void from_json(const json& j, Disruption::Option& option) { Disruption::Option::from_json(j, option); }
	};

	NLOHMANN_JSON_SERIALIZE_ENUM( Disruption::Option::Type, {
		{ Disruption::Option::Type::HERO, "hero" },
		{ Disruption::Option::Type::ATTRIBUTE, "attribute" },
	});
}
