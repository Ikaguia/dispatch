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
			changeStatus(RESTING, restingTime); break;
		case RESTING:
			if (mission.expired()) changeStatus(AVAILABLE);
			else changeStatus(AWAITING_REVIEW);
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
		txtRect.Draw(txtColor);
		txtRect.DrawLines(BLACK);
		Utils::drawTextCentered(txt, Utils::center(txtRect), raylib::Font{}, 12, WHITE, 2, true);
	}

	if (status == TRAVELLING || status == RETURNING) {
		raylib::Vector2 origin{500,200};
		raylib::Vector2 dest = (mission.lock()->position);
		if (status == RETURNING) std::swap(origin, dest);
		float pct = 1.0f * elapsedTime / finishTime;
		raylib::Vector2 travelPos = (dest - origin) * pct + origin;
		travelPos.DrawCircle(12, BLACK);
		travelPos.DrawCircle(11, WHITE);
		travelPos.DrawCircle(10, status == TRAVELLING ? BLUE : YELLOW);
		// TODO: Draw hero portrait
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
