#include <cctype>

#include <Utils.hpp>
#include <Hero.hpp>
#include <Mission.hpp>

Hero::Hero(const std::string& name, const std::map<std::string, int> &attr) : name(name), real_attributes{} {
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		real_attributes[attribute] = value;
	}
};


AttrMap<int> Hero::attributes() const {
	if (health == Health::NORMAL) return real_attributes;
	AttrMap<int> attrs = real_attributes;
	if (health == Health::WOUNDED) for (auto& [key, val] : attrs) if (val > 1) val--;
	if (health == Health::DOWNED) for (auto& [key, val] : attrs) val = 1;
	return attrs;
}

float Hero::travelSpeed() const { return travelSpeedMult * (1 + attributes()[Attribute::MOBILITY] / 5.0f); }

void Hero::update(float deltaTime) {
	if (status == TRAVELLING) elapsedTime += deltaTime * travelSpeed();
	else elapsedTime += deltaTime;

	if (elapsedTime >= finishTime) switch (status) {
		case TRAVELLING:
			changeStatus(WORKING, mission); break;
		case RETURNING:
			changeStatus(RESTING, std::weak_ptr<Mission>{}, restingTime); break;
		case RESTING:
			changeStatus(AVAILABLE); break;
		default:
			break;
	}
}

void Hero::renderUI(raylib::Vector2 pos) const {
	bool draw = true;
	raylib::Vector2 sz{93, 115};
	raylib::Color color{};
	switch(status) {
		case Hero::ASSIGNED:
			color = ColorAlpha(BLUE, 0.2f); break;
		case Hero::TRAVELLING:
		case Hero::WORKING:
		case Hero::RETURNING:
			color = ColorAlpha(GREEN, 0.1f); break;
		case Hero::RESTING:
			color = ColorAlpha(YELLOW, 0.1f); break;
		// case Hero::AVAILABLE:
		case Hero::UNAVAILABLE:
			color = ColorAlpha(LIGHTGRAY, 0.3f); break;
			break;
		default:
			draw = false;
	}
	if (health != Health::NORMAL) draw = true;
	if (health == Health::WOUNDED) color.Lerp(RED, 0.2f);
	if (health == Health::DOWNED) color.Lerp(RED, 0.4f);
	if (draw) pos.DrawRectangle(sz, color);

	if (status == TRAVELLING || status == RETURNING) {
		raylib::Vector2 origin{500,200};
		raylib::Vector2 dest = (mission.lock()->position);
		if (status == RETURNING) std::swap(origin, dest);
		float pct = 1.0f * elapsedTime / finishTime;
		raylib::Vector2 travelPos = (dest - origin) * pct + origin;
		travelPos.DrawCircle(10, BLUE);
	}
}


void Hero::changeStatus(Status st, float fnTime) { changeStatus(st, mission, fnTime); }
void Hero::changeStatus(Status st, std::weak_ptr<Mission> msn, float fnTime) {
	status = st;
	mission = msn;
	finishTime = fnTime;
	elapsedTime = 0;
}
void Hero::wound(){
	if (health == Health::NORMAL) health = Health::WOUNDED;
	else health = Health::DOWNED;
}
void Hero::heal(){
	if (health == Health::DOWNED) health = Health::WOUNDED;
	else health = Health::NORMAL;
}


bool Hero::operator<(const Hero& other) const { return name < other.name; }
