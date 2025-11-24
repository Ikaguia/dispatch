#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

class Hero;

#include <Attribute.hpp>
#include <Mission.hpp>
#include <JSONish.hpp>

class Hero : public std::enable_shared_from_this<Hero> {
public:
	std::string name, nickname, bio;
	std::vector<std::string> tags;
private:
	AttrMap<int> real_attributes;
public:
	AttrMap<int> unconfirmed_attributes;
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
	};
	enum Health {
		NORMAL,
		WOUNDED,
		DOWNED
	};
	Health health=NORMAL;
	float travelSpeedMult = 1.0f;
	float elapsedTime = 0.0f;
	float finishTime = 0.0f;
	float restingTime = 10.0f;
	bool flies = false;
	int level = 1;
	int exp = 0;
	int skillPoints = 0;
	Status status{Hero::AVAILABLE};
	std::weak_ptr<Mission> mission{};
	raylib::Vector2 pos{500, 200}, path;
	raylib::Rectangle uiRect{};

	Hero(const JSONish::Node& data);

	AttrMap<int> attributes() const;
	float travelSpeed() const;
	bool canFly() const;
	int maxExp() const;

	void update(float deltaTime);
	void renderUI(raylib::Rectangle rect);

	void changeStatus(Status st, float fnTime=0.0f);
	void changeStatus(Status st, std::weak_ptr<Mission> msn, float fnTime=0.0f);
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
};
