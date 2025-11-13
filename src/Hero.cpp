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

float Hero::travelSpeed() const { return travelSpeedMult * (30 + 1.5f*attributes()[Attribute::MOBILITY]); }

void Hero::update(float deltaTime) {
	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		std::shared_ptr<Mission> ms = (status == Hero::TRAVELLING) ? mission.lock() : nullptr;
		raylib::Vector2 dest = (status == Hero::TRAVELLING) ? ms->position : raylib::Vector2{500,200};
		float dist = pos.Distance(dest);
		float speed = travelSpeed() * deltaTime;
		if (dist <= speed) {
			pos = dest;
			if (status == Hero::TRAVELLING) {
				changeStatus(Hero::WORKING);
				if (ms->status != Mission::PROGRESS) ms->changeStatus(Mission::PROGRESS);
			}
			else changeStatus(Hero::RESTING, restingTime);
		} else pos = pos.MoveTowards(dest, speed);
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

	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		this->pos.DrawCircle(12, BLACK);
		this->pos.DrawCircle(11, WHITE);
		this->pos.DrawCircle(10, status == Hero::TRAVELLING ? BLUE : YELLOW);
		// TODO: Draw hero portrait
	}
}


void Hero::changeStatus(Status st, float fnTime) { changeStatus(st, mission, fnTime); }
void Hero::changeStatus(Status st, std::weak_ptr<Mission> msn, float fnTime) {
	status = st;
	mission = msn;
	finishTime = fnTime;
	elapsedTime = 0;
	Utils::println("changeStatus hero:{}, st:{}, msn:{}, fnTime:{}", name, StatusToString(st), msn.expired() ? "null" : msn.lock()->name, fnTime);
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


constexpr std::string_view Hero::StatusToString(Status st) {
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
constexpr std::string_view Hero::HealthToString(Health hlt) {
	if (hlt == NORMAL) return "NORMAL";
	if (hlt == WOUNDED) return "WOUNDED";
	if (hlt == DOWNED) return "DOWNED";
	throw std::invalid_argument("Invalid health");
}
