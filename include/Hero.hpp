#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

class Hero;

#include <Attribute.hpp>
#include <Mission.hpp>

class Hero {
public:
	const std::string name;
private:
	AttrMap<int> real_attributes;
public:
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
	Status status=Hero::AVAILABLE;
	std::weak_ptr<Mission> mission{};
	raylib::Vector2 pos{500, 200}, path;

	Hero(const std::string& name, const std::map<std::string,int> &attr = {}, bool flies=false, int lvl=-1);

	AttrMap<int> attributes() const;
	float travelSpeed() const;
	bool canFly() const;
	int maxExp() const;

	void update(float deltaTime);
	void renderUI(raylib::Vector2 pos) const;

	void changeStatus(Status st, float fnTime=0.0f);
	void changeStatus(Status st, std::weak_ptr<Mission> msn, float fnTime=0.0f);
	void wound();
	void heal();
	void addExp(int xp);
	bool updatePath();

	bool operator<(const Hero& other) const;

	static constexpr std::string_view StatusToString(Status st);
	static constexpr std::string_view HealthToString(Health hlt);
};
