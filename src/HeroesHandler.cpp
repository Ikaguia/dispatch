#include <string>
#include <algorithm>
#include <format>

#include <Utils.hpp>
#include <Common.hpp>
#include <Mission.hpp>
#include <Hero.hpp>
#include <HeroesHandler.hpp>
#include <MissionsHandler.hpp>
#include <UI.hpp>

#include <nlohmann/json.hpp>
using nlohmann::json;

extern float bgScale;

HeroesHandler::HeroesHandler() {
	// loadHeroes("resources/data/heroes/Heroes.json", true);
	loadHeroes("resources/data/heroes/Heroes2.json", true);
}

HeroesHandler& HeroesHandler::inst() {
	static HeroesHandler singleton;
	return singleton;
}

void HeroesHandler::loadHeroes(const std::string& filePath, bool activate) {
	Utils::println("Loading heroes from {}", filePath);
	json data = Utils::readJsonFile(filePath);
	if (!data.is_array()) throw std::runtime_error("Heroes error: Top-level JSON must be an array of heroes.");
	Utils::println("Read {} heroes", data.size());
	for (auto& hero_data : data) {
		auto hero = std::make_unique<Hero>(hero_data);
		// Utils::println("Loaded hero '{}'", hero->name);
		if (activate) roster.push_back(hero->name);
		loaded[hero->name] = std::move(hero);
	}
}


const Hero& HeroesHandler::operator[](const std::string& name) const {
	return *loaded.at(name).get();
}
Hero& HeroesHandler::operator[](const std::string& name) {
	return *loaded.at(name).get();
}

bool HeroesHandler::paused() const { return !selected.empty(); }

bool HeroesHandler::isHeroSelected(const Hero& hero) const { return hero.name == selected; }

void HeroesHandler::renderUI() {
	raylib::Rectangle heroesRect{158*bgScale, 804*bgScale, 1603*bgScale, 236*bgScale};
	raylib::Rectangle heroRect{heroesRect.x, heroesRect.y, 185*bgScale, heroesRect.height};
	std::vector<int> Ws = {180, 185, 189, 192, 192, 189, 185, 180};
	int spacing = (heroesRect.width - heroRect.width * roster.size()) / (roster.size() - 1);
	for (auto [idx, hero_name] : Utils::enumerate(roster)) {
		auto& hero = (*this)[hero_name];
		int i = std::min(idx, (roster.size()-1)-idx);
		heroRect.width = Ws[i] * bgScale;
		hero.renderUI(heroRect);
		heroRect.x += heroRect.width + spacing;
	}

	int points_available = std::accumulate(BEGEND(roster), 0, [&](int tot, const std::string& hero_name){ return tot + (*this)[hero_name].skillPoints; });
	if (points_available) {
		auto rect = Utils::anchorRect(detailsTabButton, {15.0f, 15.0f}, Utils::Anchor::topLeft, {}, Utils::AnchorType::center);
		rect.Draw(Dispatch::UI::bgMed);
		rect.DrawLines(BLACK);
		Utils::drawTextAnchored(std::to_string(points_available), rect, Utils::Anchor::center, Dispatch::UI::fontTitle, Dispatch::UI::textColor, 16.0f, 1.0f);
	}

	if (paused()) layoutHeroDetails.render();
}

void updateLayoutStatsData(Dispatch::UI::Layout& layout, const Hero& hero) {
	Dispatch::UI::Button* confirm = dynamic_cast<Dispatch::UI::Button*>(layout.elements.at("heroDetails-stats-confirm").get());
	Dispatch::UI::Button* reset = dynamic_cast<Dispatch::UI::Button*>(layout.elements.at("heroDetails-stats-reset").get());
	if (!confirm) throw std::runtime_error("Hero details layout is missing 'heroDetails-stats-confirm' element or it is of the wrong type.");
	if (!reset) throw std::runtime_error("Hero details layout is missing 'heroDetails-stats-reset' element or it is of the wrong type.");

	const auto& attrs = hero.attributes();
	const auto& unc_attrs = hero.unconfirmed_attributes;
	int totalUnconfirmed = 0;

	layout.updateSharedData("skillPoints", std::to_string(hero.skillPoints));
	layout.updateSharedData("attributes", attrs);
	for (Attribute::Value attr : Attribute::Values) {
		std::string attr_str = Utils::toLower((std::string)Attribute(attr).toString());
		std::string str_minus = std::format("heroDetails-stats-{}-minus", attr_str);
		std::string str_plus = std::format("heroDetails-stats-{}-plus", attr_str);
		Dispatch::UI::Button* minus = dynamic_cast<Dispatch::UI::Button*>(layout.elements.at(str_minus).get());
		Dispatch::UI::Button* plus = dynamic_cast<Dispatch::UI::Button*>(layout.elements.at(str_plus).get());
		if (!minus) throw std::runtime_error(std::format("Hero details layout is missing '{}' element or it is of the wrong type.", str_minus));
		if (!plus) throw std::runtime_error(std::format("Hero details layout is missing '{}' element or it is of the wrong type.", str_plus));

		int unconfirmed = unc_attrs[attr];
		totalUnconfirmed += unconfirmed;

		minus->changeStatus(unconfirmed ? Dispatch::UI::Element::Status::REGULAR : Dispatch::UI::Element::Status::DISABLED);
		plus->changeStatus(hero.skillPoints ? Dispatch::UI::Element::Status::REGULAR : Dispatch::UI::Element::Status::DISABLED);
		layout.updateSharedData(attr_str, std::to_string(attrs[attr] + unconfirmed));
	}
	auto status = totalUnconfirmed ? Dispatch::UI::Element::Status::REGULAR : Dispatch::UI::Element::Status::DISABLED;
	reset->changeStatus(status);
	confirm->changeStatus(status);
}

bool HeroesHandler::handleInput() {
	auto mission = MissionsHandler::inst().selectedMission;
	auto missionStatus = mission.expired() ? Mission::DONE : mission.lock()->status;
	if (paused()) layoutHeroDetails.handleInput();
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		raylib::Rectangle heroes{158*bgScale, 804*bgScale, 1603*bgScale, 236*bgScale};
		if (heroes.CheckCollision(mousePos)) {
			for (auto [idx, hero_name] : Utils::enumerate(roster)) {
				auto& hero = (*this)[hero_name];
				if (hero.uiRect.CheckCollision(mousePos)) {
					if (missionStatus == Mission::SELECTED) {
						if ((hero.status != Hero::AVAILABLE && hero.status != Hero::ASSIGNED) || hero.health == Hero::DOWNED) return false;
						mission.lock()->toggleHero(hero_name);
						return true;
					} else if (mission.expired()) {
						selectHero(hero_name);
						return true;
					}
				}
			}
		} else if (detailsTabButton.CheckCollision(mousePos)) {
			if (mission.expired() || !mission.lock()->isMenuOpen()) {
				auto it = std::find_if(BEGEND(roster), [&](const std::string& hero_name){ return (*this)[hero_name].skillPoints > 0; });
				if (it != roster.end()) selectHero(*it);
				else selectHero(roster[0]);
			}
		}
	}
	if (paused()) {
		auto& hero = (*this)[selected];

		if (layoutHeroDetails.dataChanged.contains("tab")) {
			if (layoutHeroDetails.sharedData["tab"] == "tab-upgrades") {
				changeTab(UPGRADE);
				return true;
			} else if (layoutHeroDetails.sharedData["tab"] == "tab-powers") {
				changeTab(POWERS);
				return true;
			} else if (layoutHeroDetails.sharedData["tab"] == "tab-info") {
				changeTab(INFO);
				return true;
			}
		} else if (layoutHeroDetails.release.contains("heroDetails-backButton")) {
			selectHero("");
			return true;
		} else if (layoutHeroDetails.release.contains("heroDetails-stats-reset")) {
			hero.resetAttributeChanges();
			updateLayoutStatsData(layoutHeroDetails, hero);
			return true;
		} else if (layoutHeroDetails.release.contains("heroDetails-stats-confirm")) {
			hero.applyAttributeChanges();
			updateLayoutStatsData(layoutHeroDetails, hero);
			return true;
		} else {
			for (auto attr : Attribute::Values) {
				std::string minus = std::format("heroDetails-stats-{}-minus", Utils::toLower((std::string)Attribute(attr).toString()));
				std::string plus = std::format("heroDetails-stats-{}-plus", Utils::toLower((std::string)Attribute(attr).toString()));
				if (layoutHeroDetails.release.contains(minus)) {
					if (hero.unconfirmed_attributes[attr] > 0) {
						hero.unconfirmed_attributes[attr]--;
						hero.skillPoints++;
					}
					updateLayoutStatsData(layoutHeroDetails, hero);
					return true;
				} else if (layoutHeroDetails.release.contains(plus)) {
					if (hero.skillPoints > 0) {
						hero.unconfirmed_attributes[attr]++;
						hero.skillPoints--;
					}
					updateLayoutStatsData(layoutHeroDetails, hero);
					return true;
				}
			}
		}
	}
	return false;
}

void HeroesHandler::update(float deltaTime) { for (auto& hero_name : roster) (*this)[hero_name].update(deltaTime); }

void HeroesHandler::selectHero(const std::string& name) {
	selected = name;
	Dispatch::UI::Image* image = dynamic_cast<Dispatch::UI::Image*>(layoutHeroDetails.elements.at("heroDetails-image").get());
	Dispatch::UI::Image* mugshot = dynamic_cast<Dispatch::UI::Image*>(layoutHeroDetails.elements.at("heroDetails-info-mugshot").get());
	if (!image) throw std::runtime_error("Hero details layout is missing 'heroDetails-image' element or it is of the wrong type.");

	if (paused()) {
		const Hero& hero = (*this)[selected];
		const AttrMap<int>& attrs = hero.attributes();
		std::vector<std::string> powerNames;
		for (const auto& power : hero.powers) powerNames.push_back(power.name);
		for (const auto& power : hero.powers) {
			layoutHeroDetails.updateSharedData("powers." + power.name + ".name", power.unlocked ? power.name : "???");
			layoutHeroDetails.updateSharedData("powers." + power.name + ".description", power.unlocked ? power.description : "???");
		}

		layoutHeroDetails.updateSharedData("name", hero.name);
		layoutHeroDetails.updateSharedData("bio", hero.bio);
		layoutHeroDetails.updateSharedData("tags", hero.tags);
		layoutHeroDetails.updateSharedData("powers", hero.powers);
		layoutHeroDetails.updateSharedData("powerNames", powerNames);
		image->texture.Load(hero.img_paths.at("full"));
		mugshot->texture.Load(hero.img_paths.at("mugshot"));
		updateLayoutStatsData(layoutHeroDetails, hero);
	}
}

void HeroesHandler::changeTab(Tab newTab) {
	tab = newTab;
	layoutHeroDetails.elements.at("heroDetails-stats").get()->visible = (tab == UPGRADE);
	layoutHeroDetails.elements.at("heroDetails-powers").get()->visible = (tab == POWERS);
	layoutHeroDetails.elements.at("heroDetails-info").get()->visible = (tab == INFO);
}
