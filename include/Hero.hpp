#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <unordered_map>

#include <Attribute.hpp>

class Power;

class Hero {
private:
	AttrMap<int> real_attributes, memo_attributes;
public:
	std::string name, nickname{"?"};
	std::vector<std::string> tags;
	std::map<std::string, std::string> bio;
	std::unordered_map<std::string, std::string> img_paths;
	AttrMap<int> unconfirmed_attributes;
	std::vector<Power> powers;
	enum Status {
		ASSIGNED,
		TRAVELLING,
		WORKING,
		DISRUPTED,
		RETURNING,
		RESTING,
		AVAILABLE,
		UNAVAILABLE,
		AWAITING_REVIEW
	} status{AVAILABLE};
	enum Health {
		NORMAL,
		WOUNDED,
		DOWNED
	} health{NORMAL};
	float travelSpeedMult=1.0f, elapsedTime=0.0f, finishTime=0.0f, restingTime=10.0f;
	bool flies=false, needsAttrCalc=true;
	int level=1, exp=0, skillPoints=3, expOffset=0;
	std::string mission;
	raylib::Vector2 pos{500, 200}, path;
	raylib::Rectangle uiRect{};

	Hero(const nlohmann::json& data);

	Hero(const Hero&) = delete;
	Hero& operator=(const Hero&) = delete;
	Hero(Hero&&) noexcept = default;
	Hero& operator=(Hero&&) noexcept = default;

	AttrMap<int> attributes();
	float travelSpeed();
	bool canFly() const;
	int maxExp() const;

	void update(float deltaTime);
	void renderUI(raylib::Rectangle rect);

	void changeStatus(Status st, float fnTime=0.0f);
	void changeStatus(Status st, std::string msn, float fnTime=0.0f);
	void wound();
	void heal();
	void addExp(int xp);
	void levelUp();
	void applyAttributeChanges();
	void resetAttributeChanges();
	bool updatePath();

	bool operator<(const Hero& other) const;

	static std::string_view StatusToString(Status st);
	static std::string_view HealthToString(Health hlt);

	static void to_json(nlohmann::json& j, const Hero& hero);
	static void from_json(const nlohmann::json& j, Hero& hero);
};

namespace nlohmann {
	template <>
	struct adl_serializer<Hero> {
		static void to_json(json& j, const Hero& hero) { Hero::to_json(j, hero); }
		static void from_json(const json& j, Hero& hero) { Hero::from_json(j, hero); }
	};

	NLOHMANN_JSON_SERIALIZE_ENUM( Hero::Status, {
		{ Hero::Status::UNAVAILABLE, "unavailable" },
		{ Hero::Status::ASSIGNED, "assigned" },
		{ Hero::Status::TRAVELLING, "travelling" },
		{ Hero::Status::WORKING, "working" },
		{ Hero::Status::DISRUPTED, "disrupted" },
		{ Hero::Status::RETURNING, "returning" },
		{ Hero::Status::RESTING, "resting" },
		{ Hero::Status::AVAILABLE, "available" },
		{ Hero::Status::AWAITING_REVIEW, "awaiting_review" },
	});
	NLOHMANN_JSON_SERIALIZE_ENUM( Hero::Health, {
		{ Hero::Health::NORMAL, "normal" },
		{ Hero::Health::WOUNDED, "wounded" },
		{ Hero::Health::DOWNED, "downed" },
	});
}

#include <Power.hpp>
