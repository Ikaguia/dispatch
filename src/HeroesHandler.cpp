#include <string>
#include <algorithm>
#include <format>

#include <Utils.hpp>
#include <Common.hpp>
#include <Mission.hpp>
#include <Hero.hpp>
#include <HeroesHandler.hpp>
#include <MissionsHandler.hpp>

extern float bgScale;

HeroesHandler::HeroesHandler() {
	active_heroes.emplace_back(new Hero{"Sonar", "Batboy Conman", "?", {"INTELLECTUAL", "HYBRID"}, {
		{"combat", 1},
		{"vigor", 2},
		{"mobility", 4},
		{"charisma", 3},
		{"intelligence", 5}
	}, true});
	active_heroes.emplace_back(new Hero{"Flambae", "Controls Fire and Flame", "He controls the fire and flames and his skin doesn't burn", {"FIRE", "FIGHTER"}, {
		{"combat", 4},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 2},
		{"intelligence", 1}
	}, true});
	active_heroes.emplace_back(new Hero{"Invisigal", "AKA Invisibitch", "?", {"THIEF", "LONER"}, {
		{"combat", 2},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 1},
		{"intelligence", 2}
	}});
	active_heroes.emplace_back(new Hero{"Punch Up", "Smallest Strongman", "?", {"FLYWEIGHT", "BRAWLER"}, {
		{"combat", 3},
		{"vigor", 4},
		{"mobility", 1},
		{"charisma", 3},
		{"intelligence", 1}
	}});
	active_heroes.emplace_back(new Hero{"Prism", "Trick of the Light", "?", {"POP STAR", "ILLUSIONIST"}, {
		{"combat", 3},
		{"vigor", 1},
		{"mobility", 3},
		{"charisma", 4},
		{"intelligence", 2}
	}});
	active_heroes.emplace_back(new Hero{"Malevola", "Devil from Down Under", "?", {"SORCERESS", "HALF-DEMON"}, {
		{"combat", 3},
		{"vigor", 2},
		{"mobility", 2},
		{"charisma", 3},
		{"intelligence", 2}
	}});
	active_heroes.emplace_back(new Hero{"Golem", "The Sentient Construct", "?", {"DEFENDER", "VERSATILE"}, {
		{"combat", 2},
		{"vigor", 4},
		{"mobility", 1},
		{"charisma", 2},
		{"intelligence", 1}
	}});
	active_heroes.emplace_back(new Hero{"Coupé", "Merciless Mercenary", "?", {"SHADOW", "ASSASSIN"}, {
		{"combat", 4},
		{"vigor", 2},
		{"mobility", 3},
		{"charisma", 1},
		{"intelligence", 2}
	}, true});
}

HeroesHandler& HeroesHandler::inst() {
	static HeroesHandler singleton;
	return singleton;
}


std::weak_ptr<const Hero> HeroesHandler::operator[](const std::string& name) const {
	auto it = std::find_if(active_heroes.begin(), active_heroes.end(), [&](const std::shared_ptr<Hero>& hero) { return hero->name == name; });
	if (it != active_heroes.end()) return std::const_pointer_cast<const Hero>(*it);
	return {};
}
std::weak_ptr<Hero> HeroesHandler::operator[](const std::string& name) {
	auto it = std::find_if(active_heroes.begin(), active_heroes.end(), [&](const std::shared_ptr<Hero>& hero) { return hero->name == name; });
	if (it != active_heroes.end()) return *it;
	return {};
}

bool HeroesHandler::paused() const { return selectedHeroIndex != -1; }

bool HeroesHandler::isHeroSelected(const std::weak_ptr<Hero> hero) const { return selectedHeroIndex != -1 && active_heroes[selectedHeroIndex] == hero.lock(); }

void HeroesHandler::renderUI() {
	raylib::Rectangle heroesRect{158*bgScale, 804*bgScale, 1603*bgScale, 236*bgScale};
	raylib::Rectangle heroRect{heroesRect.x, heroesRect.y, 185*bgScale, heroesRect.height};
	std::vector<int> Ws = {180, 185, 189, 192, 192, 189, 185, 180};
	int spacing = (heroesRect.width - heroRect.width * active_heroes.size()) / (active_heroes.size() - 1);
	for (auto [idx, hero] : Utils::enumerate(active_heroes)) {
		int i = std::min(idx, (active_heroes.size()-1)-idx);
		heroRect.width = Ws[i] * bgScale;
		hero->renderUI(heroRect);
		heroRect.x += heroRect.width + spacing;
	}

	if (selectedHeroIndex != -1) {
		auto& hero = active_heroes[selectedHeroIndex];

		extern raylib::Window window;

		// MAIN FRAME
		raylib::Rectangle mainRect{ 34.0f, 34.0f, window.GetWidth() - 68.0f, 355.0f };

		// ==========
		// TITLE BAR
		// ==========
		{
			std::vector<std::tuple<HeroesHandler::Tabs, std::string>> tabs{
				{ UPGRADE, "UPGRADE" },
				{ POWERS, "POWERS" },
				{ INFO, "INFO" }
			};
			for (auto [idx, tb] : Utils::enumerate(tabs)) {
				auto& [tabEnum, tabName] = tb;
				auto& tabRect = btns[tabName];
				tabRect = raylib::Rectangle{ mainRect.x + (1+idx) * mainRect.width / 5 + 10, mainRect.y + 5, mainRect.width / 5 - 20, 30 };
				if (tab == tabEnum) tabRect.DrawGradient(ORANGE, ORANGE, ORANGE, ColorLerp(ORANGE, YELLOW, 0.7));
				else tabRect.Draw(ColorLerp(LIME, DARKBLUE, 0.4f));
				tabRect.DrawLines(BLACK);
				Utils::drawTextCentered(tabName, Utils::center(tabRect), Dispatch::UI::fontTitle, 24);
			}
		}

		// ==================
		// LEFT/CENTER PANEL
		// ==================
		raylib::Rectangle lmRect{ mainRect.x + 40, mainRect.y + 40, mainRect.GetWidth()*3/5.0f, mainRect.GetHeight() - 60 };
		{
			raylib::Rectangle(lmRect.GetPosition()+raylib::Vector2{5,5}, lmRect.GetSize()).Draw(Dispatch::UI::shadow);
			raylib::Rectangle ilmRect = Utils::inset(lmRect, 2); ilmRect.height -= 34;
			lmRect.DrawGradient(Dispatch::UI::bgMed, Dispatch::UI::bgMed, Dispatch::UI::bgDrk, Dispatch::UI::bgMed);
			ilmRect.Draw(Dispatch::UI::bgLgt);

			// BACK BUTTON
			auto& btnRect = btns["BACK"];
			btnRect = raylib::Rectangle{Utils::center(ilmRect).x - 40, ilmRect.y + ilmRect.height + 2, 80, 30};
			btnRect.Draw(Dispatch::UI::bgLgt);
			btnRect.DrawLines(BLACK);
			Utils::drawTextCentered("BACK", Utils::center(btnRect), Dispatch::UI::fontText, 20, Dispatch::UI::textColor);

			// ================================================
			// LEFT PANEL — HERO PORTRAIT + NAME + RANK + TAGS
			// ================================================
			{
				// (TODO: draw hero->portrait texture here)

				// TAGS (class, traits)
				raylib::Rectangle tagRect{ilmRect.x + 20, ilmRect.y + ilmRect.height - 40, 100, 30};
				for (auto& tag : hero->tags) {
					tagRect.width = Dispatch::UI::fontText.MeasureText(tag, 16, 2).x + 20;
					tagRect.Draw(Dispatch::UI::bgLgt);
					tagRect.DrawLines(BLACK);
					Utils::drawTextCentered(tag, Utils::center(tagRect), Dispatch::UI::fontText, 16, Dispatch::UI::textColor);
					tagRect.x += tagRect.width + 10;
				}

				// NICKNAME + RANK
				raylib::Vector2 infoPos{ilmRect.x + 20, tagRect.y - 10};
				std::string info = std::format("RANK {}", hero->level);
				if (hero->nickname != "") info = std::format("{} | {}", hero->nickname, info);
				infoPos.y -= Dispatch::UI::fontText.MeasureText(info, 18, 1).y;
				Dispatch::UI::fontText.DrawText(info, infoPos, 18, 1, Dispatch::UI::textColor);

				// HERO NAME
				raylib::Vector2 namePos{ilmRect.x + 20, infoPos.y - 10};
				namePos.y -= Dispatch::UI::fontTitle.MeasureText(hero->name, 26, 1).y;
				Dispatch::UI::fontTitle.DrawText(hero->name, namePos, 26, 1, Dispatch::UI::textColor);
			}

			// =============
			// CENTER PANEL
			// =============
			raylib::Rectangle centerRect{ ilmRect.x + ilmRect.width - 240, ilmRect.y + 10, 220, ilmRect.height - 20 };
			{
				if (tab == UPGRADE) {
					raylib::Vector2 spPos{centerRect.x + 10, centerRect.y + 10};
					std::string sp = "Skill Points Unspent: " + std::to_string(hero->skillPoints);
					Dispatch::UI::fontText.DrawText(sp, spPos, 18, 1, Dispatch::UI::textColor);

					// ATTRIBUTE LIST
					raylib::Rectangle attrRect{spPos.x, spPos.y + 24, centerRect.width - 20, 28};
					int totalUnconfirmed = 0;
					for (auto attr : Attribute::Values) {
						attrRect.DrawGradient(Dispatch::UI::bgLgt, Dispatch::UI::bgLgt, Dispatch::UI::bgMed, Dispatch::UI::bgLgt);
						attrRect.DrawLines(BLACK);

						Dispatch::UI::fontText.DrawText(std::string{Attribute(attr).toString()}, {attrRect.x + 10, attrRect.y + 6}, 18, 1, Dispatch::UI::textColor);
						int unconfirmed = hero->unconfirmed_attributes[attr];
						totalUnconfirmed += unconfirmed;

						// +/- buttons
						auto& minus = btns[std::string{std::format("{}_minus", Attribute(attr).toString())}];
						auto& plus = btns[std::string{std::format("{}_plus", Attribute(attr).toString())}];
						minus = raylib::Rectangle{ attrRect.x + attrRect.width - 72, attrRect.y + 4, 22, 20 };
						raylib::Rectangle value  { attrRect.x + attrRect.width - 48, attrRect.y + 4, 22, 20 };
						plus  = raylib::Rectangle{ attrRect.x + attrRect.width - 24, attrRect.y + 4, 22, 20 };
						minus.Draw(unconfirmed      ? Dispatch::UI::bgLgt : Dispatch::UI::bgDrk); minus.DrawLines(BLACK);
						std::string val = std::to_string(hero->attributes()[attr] + hero->unconfirmed_attributes[attr]);
						plus.Draw(hero->skillPoints ? Dispatch::UI::bgLgt : Dispatch::UI::bgDrk); plus.DrawLines(BLACK);
						Utils::drawTextCentered("-", Utils::center(minus), Dispatch::UI::fontText, 18, Dispatch::UI::textColor);
						Utils::drawTextCentered(val, Utils::center(value), Dispatch::UI::fontText, 18, Dispatch::UI::textColor);
						Utils::drawTextCentered("+", Utils::center(plus),  Dispatch::UI::fontText, 18, Dispatch::UI::textColor);

						attrRect.y += 34;
					}
					auto& btnReset = btns["RESET"];
					auto& btnConfirm = btns["CONFIRM"];
					btnReset = raylib::Rectangle{attrRect.x, attrRect.y, attrRect.width / 2 - 1, attrRect.height};
					btnConfirm = raylib::Rectangle{btnReset.x + btnReset.width + 2, btnReset.y, btnReset.width, btnReset.height};
					btnReset.Draw(totalUnconfirmed ? Dispatch::UI::bgLgt : Dispatch::UI::bgDrk); btnReset.DrawLines(BLACK);
					btnConfirm.Draw(totalUnconfirmed ? Dispatch::UI::bgLgt : Dispatch::UI::bgDrk); btnConfirm.DrawLines(BLACK);
					Utils::drawTextCentered("RESET", Utils::center(btnReset), Dispatch::UI::fontText, 20, Dispatch::UI::textColor);
					Utils::drawTextCentered("CONFIRM", Utils::center(btnConfirm), Dispatch::UI::fontText, 20, Dispatch::UI::textColor);
				} else if (tab == POWERS) {
				} else if (tab == INFO) {
					std::string bio = Utils::addLineBreaks(hero->bio, centerRect.width - 20, Dispatch::UI::fontText, 18, 2);
					Dispatch::UI::fontText.DrawText(bio, {centerRect.x + 10, centerRect.y + 10}, 18, 2, Dispatch::UI::textColor);
				}
			}
		}

		// ===============================
		// RIGHT PANEL — RADAR & SYNERGIES
		// ===============================
		raylib::Rectangle rightRect{ lmRect.x + lmRect.width + 15, lmRect.y, (mainRect.x + mainRect.width - 40) - (lmRect.x + lmRect.width + 15), lmRect.height };
		{
			raylib::Rectangle(rightRect.GetPosition()+raylib::Vector2{5,5}, rightRect.GetSize()).Draw(Dispatch::UI::shadow);
			rightRect.Draw(Dispatch::UI::bgMed);
			rightRect = Utils::inset(rightRect, 2);

			// TODO: Synergies
			if (false) {
				raylib::Rectangle synRect{ rightRect.x, rightRect.y + rightRect.height - 80, rightRect.width, 80 };
				synRect.DrawLines(BLACK);
				rightRect.height -= 82;
			}

			// RADAR GRAPH
			raylib::Rectangle radarRect{ rightRect };
			radarRect.Draw(Dispatch::UI::bgLgt);
			radarRect.DrawLines(BLACK);
			auto heroAttrs = hero->attributes() + hero->unconfirmed_attributes;
			std::vector<std::tuple<AttrMap<int>, raylib::Color, bool>> attrs{{ heroAttrs, ORANGE, true }};
			Utils::drawRadarGraph(Utils::center(radarRect), 60, attrs, Dispatch::UI::textColor, BROWN);
		}
	}
}

bool HeroesHandler::handleInput() {
	auto mission = MissionsHandler::inst().selectedMission;
	auto missionStatus = mission.expired() ? Mission::DONE : mission.lock()->status;
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		raylib::Rectangle heroes{158*bgScale, 804*bgScale, 1603*bgScale, 236*bgScale};
		if (heroes.CheckCollision(mousePos)) {
			for (auto [idx, hero] : Utils::enumerate(active_heroes)) {
				if (hero->uiRect.CheckCollision(mousePos)) {
					if (missionStatus == Mission::SELECTED) {
						if ((hero->status != Hero::AVAILABLE && hero->status != Hero::ASSIGNED) || hero->health == Hero::DOWNED) return false;
						mission.lock()->toggleHero(hero);
						return true;
					} else if (mission.expired()) {
						selectedHeroIndex = idx;
						return true;
					}
				}
			}
		} else if (selectedHeroIndex != -1) {
			auto& hero = active_heroes[selectedHeroIndex];
			if (btns["BACK"].CheckCollision(mousePos)) {
				selectedHeroIndex = -1;
				return true;
			} else if (btns["UPGRADE"].CheckCollision(mousePos)) {
				tab = UPGRADE;
				return true;
			} else if (btns["POWERS"].CheckCollision(mousePos)) {
				tab = POWERS;
				return true;
			} else if (btns["INFO"].CheckCollision(mousePos)) {
				tab = INFO;
				return true;
			} else if (btns["RESET"].CheckCollision(mousePos)) {
				hero->resetAttributeChanges();
				return true;
			} else if (btns["CONFIRM"].CheckCollision(mousePos)) {
				hero->applyAttributeChanges();
				return true;
			} else {
				for (auto attr : Attribute::Values) {
					auto& minus = btns[std::string{std::format("{}_minus", Attribute(attr).toString())}];
					auto& plus = btns[std::string{std::format("{}_plus", Attribute(attr).toString())}];
					if (minus.CheckCollision(mousePos)) {
						if (hero->unconfirmed_attributes[attr] > 0) {
							hero->unconfirmed_attributes[attr]--;
							hero->skillPoints++;
						}
						return true;
					} else if (plus.CheckCollision(mousePos)) {
						if (hero->skillPoints > 0) {
							hero->unconfirmed_attributes[attr]++;
							hero->skillPoints--;
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}

void HeroesHandler::update(float deltaTime) { for (auto& hero : active_heroes) hero->update(deltaTime); }
