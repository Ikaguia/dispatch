#include <cctype>
#include <tuple>
#include <memory>

#include <Utils.hpp>
#include <Common.hpp>
#include <Hero.hpp>
#include <Mission.hpp>
#include <CityMap.hpp>
#include <HeroesHandler.hpp>

Hero::Hero(const JSONish::Node& data) {
	name = data.get<std::string>("name");
	Utils::println("Initializing hero", name);
	nickname = data.get<std::string>("nickname", "?");
	if (data.has("bio")) {
		auto& dataBio = data["bio"];
		for (const auto& [key, val] : dataBio.obj) bio[key] = dataBio.get<std::string>(key);
	} // else throw std::invalid_argument("Hero 'bio' cannot be empty");

	flies = data.get<bool>("flies", false);

	travelSpeedMult = data.get<float>("travelSpeedMult", 1.0f);
	restingTime = data.get<float>("restingTime", 10.0f);

	level = data.get<int>("level", 1);
	exp = data.get<int>("exp", 0);
	skillPoints = data.get<int>("skillPoints", 0);

	if (data.has("tags")) for (auto& tag : data["tags"].arr) tags.push_back(tag.sval);
	if (data.has("attributes")) {
		auto& attributes = data["attributes"];
		for (Attribute attr : Attribute::Values) real_attributes[attr] = attributes.get<int>(Utils::toLower((std::string)attr.toString()));
	} else throw std::invalid_argument("Hero 'attributes' cannot be empty");
	if (data.has("powers")) {
		auto& pwrs = data["powers"];
		for (const auto& pwrData : pwrs.arr) powers.emplace_back(pwrData, (int)powers.size() == 0);
	} // else throw std::invalid_argument("Hero 'powers' cannot be empty");

	std::string path_full = std::format("resources/images/heroes/{}/full-image.png", name);
	std::string path_portrait = std::format("resources/images/heroes/{}/base-portrait.png", name);
	std::string path_wounded = std::format("resources/images/heroes/{}/wounded-portrait.png", name);
	std::string path_mugshot = std::format("resources/images/heroes/{}/mugshot.jpg", name);
	if (data.has("images")) {
		auto& imagesData = data["images"];
		path_full = imagesData.get<std::string>("full", path_full);
		path_portrait = imagesData.get<std::string>("portrait", path_portrait);
		path_wounded = imagesData.get<std::string>("wounded", path_wounded);
		path_mugshot = imagesData.get<std::string>("mugshot", path_mugshot);
	}
	imgs["full"] = raylib::Texture(path_full);
	imgs["portrait"] = raylib::Texture(path_portrait);
	imgs["wounded"] = raylib::Texture(path_wounded);
	imgs["mugshot"] = raylib::Texture(path_mugshot);
}

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
		for (auto& hero : heroes) if (hero->name == "Sonar") return true;
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

void Hero::renderUI(raylib::Rectangle rect) {
	uiRect = rect;
	raylib::Color color{GRAY}, txtColor{};
	std::string txt;
	float progress = 1.0f;
	if (HeroesHandler::inst().isHeroSelected(shared_from_this())) color = SKYBLUE;
	else switch(status) {
		case Hero::ASSIGNED:
			color = ORANGE;
			break;
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
			color = LIGHTGRAY;
			break;
		default:
			color = Dispatch::UI::bgDrk;
			break;
	}

	raylib::Rectangle pictureRect = Utils::inset(rect, 2.0f); pictureRect.height -= 13.0f;
	std::string portrait = health == Health::NORMAL ? "portrait" : "wounded";
	if (imgs.count(portrait)) {
		pictureRect.Draw(color);
		pictureRect.DrawLines(BLACK);
		DrawTexturePro(imgs[portrait], {0.0f, 0.0f, (float)imgs[portrait].width, (float)imgs[portrait].height}, pictureRect, {0.0f, 0.0f}, 0.0f, WHITE);
	}

	if (health == Health::WOUNDED) pictureRect.Draw(ColorAlpha(RED, 0.2f));
	if (health == Health::DOWNED) pictureRect.Draw(ColorAlpha(RED, 0.4f));

	if (!txt.empty()) {
		raylib::Rectangle txtRect = Utils::inset(pictureRect, 2.0f); txtRect.height = 20;
		raylib::Rectangle txtRect1{txtRect.x, txtRect.y, progress * txtRect.width, txtRect.height};
		raylib::Rectangle txtRect2{txtRect.x + (progress * txtRect.width), txtRect.y, (1-progress) * txtRect.width, txtRect.height};
		txtRect1.Draw(txtColor);
		txtRect2.Draw(LIGHTGRAY);
		txtRect.DrawLines(BLACK);
		Utils::drawTextCentered(txt, Utils::center(txtRect), Dispatch::UI::defaultFont, 12, WHITE, 2, true);
	}

	raylib::Rectangle nameRect = Utils::positionTextAnchored(name, rect, Utils::Anchor::bottomLeft, Dispatch::UI::fontTitle, 14.0f, 2.0f, {2.0f, -2.0f});
	nameRect.width = rect.width - 4.0f; nameRect.Draw(Dispatch::UI::bgMed);
	Utils::drawTextAnchored(name, rect, Utils::Anchor::bottom, Dispatch::UI::fontTitle, Dispatch::UI::textColor, 14.0f, 2.0f, {0.0f, -2.0f});

	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		pos.DrawCircle(22, BLACK);
		pos.DrawCircle(21, WHITE);
		pos.DrawCircle(20, status == Hero::TRAVELLING ? BLUE : YELLOW);
		Utils::drawCircularTexture(imgs[portrait], pos, 20.0f, 2.0f);
	}

	raylib::Vector2 xpPos{rect.x + rect.width - 17, rect.y + rect.height - 27};
	Utils::drawFilledCircleVertical(xpPos, 12, (float)exp / maxExp(), DARKGRAY, GOLD, WHITE);
	Utils::drawTextCentered("★", xpPos+raylib::Vector2{0.0f,2.0f}, Dispatch::UI::symbolsFont, 32, WHITE);

	raylib::Vector2 attrPos{rect.x + 17, rect.y + rect.height - 27};
	Utils::drawRadarGraph(attrPos, 16, {std::tuple<AttrMap<int>, raylib::Color, bool>{attributes(), ORANGE, false}}, BLACK, ColorAlpha(BROWN, 0.4f), false);
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
