#include <cctype>
#include <tuple>
#include <memory>

#include <Utils.hpp>
#include <Common.hpp>
#include <Hero.hpp>
#include <Mission.hpp>
#include <CityMap.hpp>

Hero::Hero(const std::string& name, const std::string& nickname, const std::string& bio, const std::vector<std::string>& tags, const std::map<std::string,int> &attr, bool flies, int lvl) :
	name{name},
	nickname{nickname},
	bio{bio},
	tags{tags},
	real_attributes{},
	flies{flies}
{
	int total = 0;
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		real_attributes[attribute] = value;
		total += value;
	}
	total += skillPoints;
	level = (lvl == -1) ? std::max(total - 11, 1) : lvl;
};


AttrMap<int> Hero::attributes() const {
	if (health == Health::NORMAL) return real_attributes;
	AttrMap<int> attrs = real_attributes;
	for (auto& [key, val] : attrs) {
		if (health == Health::WOUNDED) if (val > 1) val--;
		if (health == Health::DOWNED) val = 1;
	}
	return attrs;
}

float Hero::travelSpeed() const { return travelSpeedMult * (50 + 2.5f*attributes()[Attribute::MOBILITY]); }

bool Hero::canFly() const {
	if (flies) return true;
	if (!mission.expired()) {
		auto heroes = mission.lock()->assignedHeroes;
		for (auto& hero : heroes) {
			if (hero->name == "Sonar") return true;
		}
	}
	return false;
}

int Hero::maxExp() const { return 1000 * (1 + ((level-1) * (level-1) / 10)); }

void Hero::update(float deltaTime) {
	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		float dist = pos.Distance(path);
		float speed = travelSpeed() * deltaTime;
		if (dist <= speed) {
			pos = path;
			bool completed = updatePath();
			if (completed) {
				if (status == Hero::TRAVELLING) {
					std::shared_ptr<Mission> ms = mission.lock();
					changeStatus(Hero::WORKING);
					if (ms->status != Mission::PROGRESS) ms->changeStatus(Mission::PROGRESS);
				}
				else changeStatus(Hero::RESTING, restingTime);
			}
		} else pos = pos.MoveTowards(path, speed);
	} else elapsedTime += deltaTime;

	if (elapsedTime >= finishTime) switch (status) {
		case Hero::RESTING:
			if (mission.expired()) changeStatus(Hero::AVAILABLE);
			else changeStatus(Hero::AWAITING_REVIEW);
			break;
		default:
			break;
	}
}

void Hero::renderUI(raylib::Vector2 pos) const {
	bool draw = true;
	raylib::Rectangle rect{pos.x, pos.y, 93, 115};
	raylib::Color color{ColorAlpha(GRAY, 0.4f)}, txtColor{};
	std::string txt;
	float progress = 1.0f;
	switch(status) {
		case Hero::ASSIGNED:
			color = ColorAlpha(ORANGE, 0.15f); break;
		case Hero::TRAVELLING:
		case Hero::WORKING:
			txt = "BUSY";
			txtColor = SKYBLUE;
			break;
		case Hero::DISRUPTED:
			txt = "DISRUPTED";
			txtColor = RED;
			break;
		case Hero::RETURNING:
			txt = "RETURNING";
			txtColor = ColorLerp(YELLOW, ORANGE, 0.5f);
			break;
		case Hero::RESTING:
			txt = "RESTING";
			txtColor = ColorLerp(LIME, SKYBLUE, 0.6f);
			progress = 1.0f - (elapsedTime / restingTime);
			break;
		case Hero::AWAITING_REVIEW:
			txt = "AWAITING REVIEW";
			txtColor = ColorLerp(LIGHTGRAY, SKYBLUE, 0.6f);
			break;
		// case Hero::AVAILABLE:
		case Hero::UNAVAILABLE:
			txt = "UNAVAILABLE";
			txtColor = GRAY;
			color = ColorAlpha(LIGHTGRAY, 0.3f); break;
		default:
			draw = false;
	}
	if (draw) rect.Draw(color);
	if (health == Health::WOUNDED) rect.Draw(ColorAlpha(RED, 0.2f));
	if (health == Health::DOWNED) rect.Draw(ColorAlpha(RED, 0.4f));

	if (!txt.empty()) {
		raylib::Rectangle txtRect{rect.x+3, rect.y+3, rect.width-6, 20};
		raylib::Rectangle txtRect1{txtRect.x, txtRect.y, progress * txtRect.width, txtRect.height};
		raylib::Rectangle txtRect2{txtRect.x + (progress * txtRect.width), txtRect.y, (1-progress) * txtRect.width, txtRect.height};
		txtRect1.Draw(txtColor);
		txtRect2.Draw(LIGHTGRAY);
		txtRect.DrawLines(BLACK);
		Utils::drawTextCentered(txt, Utils::center(txtRect), Dispatch::UI::defaultFont, 12, WHITE, 2, true);
	}

	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		this->pos.DrawCircle(12, BLACK);
		this->pos.DrawCircle(11, WHITE);
		this->pos.DrawCircle(10, status == Hero::TRAVELLING ? BLUE : YELLOW);
		// TODO: Draw hero portrait
	}

	raylib::Vector2 xpPos{rect.x + rect.width - 17, rect.y + rect.height - 25};
	Utils::drawFilledCircleVertical(xpPos, 10, (float)exp / maxExp(), DARKGRAY, GOLD, WHITE);
	Utils::drawTextCentered("*", xpPos, Dispatch::UI::defaultFont, 16, WHITE);

	raylib::Vector2 attrPos{rect.x + 17, rect.y + rect.height - 25};
	Utils::drawRadarGraph(attrPos, 10, {std::tuple<AttrMap<int>, raylib::Color, bool>{attributes(), ORANGE, false}}, BLACK, BROWN, false);
}


void Hero::changeStatus(Status st, float fnTime) { changeStatus(st, mission, fnTime); }
void Hero::changeStatus(Status st, std::weak_ptr<Mission> msn, float fnTime) {
	status = st;
	mission = msn;
	finishTime = fnTime;
	elapsedTime = 0;
	updatePath();
}
void Hero::wound(){
	if (health == Health::NORMAL) health = Health::WOUNDED;
	else health = Health::DOWNED;
}
void Hero::heal(){
	if (health == Health::DOWNED) health = Health::WOUNDED;
	else health = Health::NORMAL;
}
void Hero::addExp(int xp) {
	exp += xp;
	while (exp >= maxExp()) {
		exp -= maxExp();
		levelUp();
	}
}
void Hero::levelUp() {
	level++;
	skillPoints++;
}
void Hero::applyAttributeChanges() {
	for (int i = 0; i < Attribute::COUNT; i++) {
		Attribute attr = Attribute::Values[i];
		real_attributes[attr] += unconfirmed_attributes[attr];
		unconfirmed_attributes[attr] = 0;
	}
}
void Hero::resetAttributeChanges() {
	for (int i = 0; i < Attribute::COUNT; i++) {
		Attribute attr = Attribute::Values[i];
		skillPoints += unconfirmed_attributes[attr];
		unconfirmed_attributes[attr] = 0;
	}
}
bool Hero::updatePath() {
	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		CityMap& cityMap = CityMap::inst();
		raylib::Vector2 dest = (status == Hero::TRAVELLING) ? mission.lock()->position : cityMap.points[89];

		int posIDX = cityMap.closestPoint(pos);
		int destIDX = cityMap.closestPoint(dest);
		int pathIDX = cityMap.shortestPath(posIDX, destIDX);

		if (path == raylib::Vector2{0,0}) {
			if (status == Hero::TRAVELLING) posIDX = 89;
			path = cityMap.points[posIDX];
		} else if (canFly()) {
			path = dest;
			return pos == dest;
		} else if (posIDX == destIDX) {
			if (pos == dest) {
				path = raylib::Vector2{0,0};
				return true;
			}
			path = dest;
		} else {
			path = cityMap.points[pathIDX];
		}
	}
	return false;
}


bool Hero::operator<(const Hero& other) const { return name < other.name; }


std::string_view Hero::StatusToString(Status st) {
	if (st == ASSIGNED) return "ASSIGNED";
	if (st == TRAVELLING) return "TRAVELLING";
	if (st == WORKING) return "WORKING";
	if (st == DISRUPTED) return "DISRUPTED";
	if (st == RETURNING) return "RETURNING";
	if (st == RESTING) return "RESTING";
	if (st == AVAILABLE) return "AVAILABLE";
	if (st == UNAVAILABLE) return "UNAVAILABLE";
	if (st == AWAITING_REVIEW) return "AWAITING_REVIEW";
	throw std::invalid_argument("Invalid status");
}
std::string_view Hero::HealthToString(Health hlt) {
	if (hlt == NORMAL) return "NORMAL";
	if (hlt == WOUNDED) return "WOUNDED";
	if (hlt == DOWNED) return "DOWNED";
	throw std::invalid_argument("Invalid health");
}
