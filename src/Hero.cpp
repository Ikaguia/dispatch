#include <cctype>
#include <tuple>
#include <memory>
#include <typeinfo>

#include <Utils.hpp>
#include <Common.hpp>
#include <Hero.hpp>
#include <Mission.hpp>
#include <CityMap.hpp>
#include <HeroesHandler.hpp>
#include <PowersManager.hpp>
#include <Attribute.hpp>
#include <MissionsHandler.hpp>
#include <TextureManager.hpp>
#include <PowersManager.hpp>

using nlohmann::json;

Hero::Hero(const json& data) {
	Utils::println("Initializing hero", data.at("name").get<std::string>());
	for (auto attr : Attribute::Values) unconfirmed_attributes[attr] = 0;
	Hero::from_json(data, *this);
}

AttrMap<int> Hero::attributes() {
	if (needsAttrCalc) {
		memo_attributes = real_attributes;
		auto& pm = PowersManager::inst();
		EventData ed;
		HeroCalcAttrData ad;
		ad.name = name;
		for (auto& [key, val] : memo_attributes) {
			ad.attr = key;
			ad.val = &val;
			ed = ad;
			pm.call(Event::HeroCalcAttr, ed, {name});
			if (health == Health::WOUNDED) { if (val > 1) val--; }
			else if (health == Health::DOWNED) val = 1;
		}
		needsAttrCalc = false;
	}
	return memo_attributes;
}

float Hero::travelSpeed() { return travelSpeedMult * (50 + 2.5f*attributes()[Attribute::MOBILITY]); }

bool Hero::canFly() const {
	if (flies) return true;
	if (!mission.empty()) {
		auto heroes = MissionsHandler::inst()[mission].assignedHeroes;
		if (heroes.contains("Sonar")) return true;
	}
	return false;
}

int Hero::maxExp() const { return 700 + 300 * level + expOffset; }

void Hero::update(float deltaTime) {
	if (status == Hero::TRAVELLING || status == Hero::RETURNING) {
		float dist = pos.Distance(path);
		float speed = travelSpeed() * deltaTime;
		if (dist <= speed) {
			pos = path;
			bool completed = updatePath();
			if (completed) {
				if (status == Hero::TRAVELLING) {
					Mission& ms = MissionsHandler::inst()[mission];
					changeStatus(Hero::WORKING);
					if (ms.status != Mission::PROGRESS) ms.changeStatus(Mission::PROGRESS);
				}
				else changeStatus(Hero::RESTING, restingTime);
			}
		} else pos = pos.MoveTowards(path, speed);
	} else elapsedTime += deltaTime;

	if (elapsedTime >= finishTime) switch (status) {
		case Hero::RESTING:
			if (mission.empty()) changeStatus(Hero::AVAILABLE);
			else changeStatus(Hero::AWAITING_REVIEW);
			break;
		default:
			break;
	}
}

void Hero::renderUI(raylib::Rectangle rect) {
	auto& TM = TextureManager::inst();
	uiRect = rect;
	raylib::Color color{GRAY}, txtColor{};
	std::string txt;
	float progress = 1.0f;
	if (HeroesHandler::inst().isHeroSelected(name)) color = SKYBLUE;
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
	std::string img_key = std::format("hero-{}-{}", name, portrait);
	if (TM.has(img_key)) {
		raylib::Texture& img = TM[img_key];
		pictureRect.Draw(color);
		pictureRect.DrawLines(BLACK);
		DrawTexturePro(img, {0.0f, 0.0f, (float)img.width, (float)img.height}, pictureRect, {0.0f, 0.0f}, 0.0f, WHITE);
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
		if (TM.has(img_key)) {
			raylib::Texture& img = TM[img_key];
			Utils::drawCircularTexture(img, pos, 20.0f, 2.0f);
		}
	}

	raylib::Vector2 xpPos{rect.x + rect.width - 17, rect.y + rect.height - 27};
	Utils::drawFilledCircleVertical(xpPos, 12, (float)exp / maxExp(), DARKGRAY, GOLD, WHITE);
	Utils::drawTextCentered("★", xpPos+raylib::Vector2{0.0f,2.0f}, Dispatch::UI::symbolsFont, 32, WHITE);

	raylib::Vector2 attrPos{rect.x + 17, rect.y + rect.height - 27};
	Utils::drawRadarGraph(attrPos, 16, {std::tuple<AttrMap<int>, raylib::Color, bool>{attributes(), ORANGE, false}}, BLACK, ColorAlpha(BROWN, 0.4f), false);
}


void Hero::changeStatus(Status st, float fnTime) { changeStatus(st, mission, fnTime); }
void Hero::changeStatus(Status st, std::string msn, float fnTime) {
	status = st;
	mission = msn;
	finishTime = fnTime;
	elapsedTime = 0;
	updatePath();
}
void Hero::wound(){
	if (health != Health::DOWNED) needsAttrCalc = true;
	if (health == Health::NORMAL) health = Health::WOUNDED;
	else health = Health::DOWNED;
}
void Hero::heal(){
	if (health != Health::NORMAL) needsAttrCalc = true;
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
	needsAttrCalc = true;
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
		raylib::Vector2 dest = (status == Hero::TRAVELLING) ? MissionsHandler::inst()[mission].position : cityMap.points[89];

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


#define READ(j, var)		hero.var = j.value(#var, hero.var)
#define READREQ(j, var) { \
							if (j.contains(#var)) hero.var = j.at(#var).get<decltype(hero.var)>(); \
							else throw std::invalid_argument("Hero '" + std::string(#var) + "' cannot be empty"); \
						}
#define READ2(j, var, key)	hero.var = j.value(#key, hero.var)
#define READREQ2(j, var, key) { \
							if (j.contains(#key)) hero.var = j.at(#key).get<decltype(hero.var)>(); \
							else throw std::invalid_argument("Hero '" + std::string(#key) + "' cannot be empty"); \
						}

void Hero::to_json(nlohmann::json& j, const Hero& hero) {
	j = json{
		{"name", hero.name},
		{"nickname", hero.nickname},
		{"tags", hero.tags},
		{"bio", hero.bio},
		{"images", hero.img_paths},
		{"attributes", hero.real_attributes},
		// {"unconfirmed_attributes", hero.unconfirmed_attributes},
		{"powers", json::array()},
		// {"status", hero.status},
		{"health", hero.health},
		{"travelSpeedMult", hero.travelSpeedMult},
		// {"elapsedTime", hero.elapsedTime},
		{"finishTime", hero.finishTime},
		{"restingTime", hero.restingTime},
		{"flies", hero.flies},
		{"level", hero.level},
		{"exp", hero.exp},
		{"expOffset", hero.expOffset},
		{"skillPoints", hero.skillPoints},
		// {"mission", hero.mission.lock()->name},
		// {"pos", hero.pos},
		// {"path", hero.path},
	};
	auto& ph = PowersManager::inst();
	for (auto& key : hero.powers) {
		auto& power = ph[key];
		j["powers"].emplace_back(power);
	}
}
void Hero::from_json(const nlohmann::json& j, Hero& hero) {
	READREQ(j, name);
	READ(j, nickname);
	READ(j, tags);
	READ(j, bio);

	hero.img_paths["full"] = std::format("resources/images/heroes/{}/full-image.png", hero.name);
	hero.img_paths["portrait"] = std::format("resources/images/heroes/{}/base-portrait.png", hero.name);
	hero.img_paths["wounded"] = std::format("resources/images/heroes/{}/wounded-portrait.png", hero.name);
	hero.img_paths["mugshot"] = std::format("resources/images/heroes/{}/mugshot.jpg", hero.name);
	if (j.contains("images")) {
		auto& imagesData = j["images"];
		READ2(imagesData, img_paths["full"], full);
		READ2(imagesData, img_paths["portrait"], portrait);
		READ2(imagesData, img_paths["wounded"], wounded);
		READ2(imagesData, img_paths["mugshot"], mugshot);
	}
	auto& TM = TextureManager::inst();
	for (auto& [type, path] : hero.img_paths) TM.load(path, std::format("hero-{}-{}", hero.name, type));

	READREQ2(j, real_attributes, attributes);
	// READ(j, unconfirmed_attributes);
	if (j.contains("powers")) {
		auto& ph = PowersManager::inst();
		for (auto& jp : j["powers"]) {
			auto pName = jp.at("name").get<std::string>();
			auto key = std::format("{}-{}", hero.name, pName);
			ph.load(jp, key);
			ph[key].hero = hero.name;
			hero.powers.push_back(key);
		}
	}
	// READ(j, status);
	READ(j, health);
	READ(j, travelSpeedMult);
	// READ(j, elapsedTime);
	READ(j, finishTime);
	READ(j, restingTime);
	READ(j, flies);
	READ(j, level);
	READ(j, exp);
	READ(j, expOffset);
	READ(j, skillPoints);
	// READ(j, mission);
	// READ(j, pos);
	// READ(j, path);
}
