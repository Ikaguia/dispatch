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
		heroes[hero->name] = std::move(hero);
	}
}


const Hero* HeroesHandler::get(const std::string& name) const { return (heroes.at(name).get()); }
Hero* HeroesHandler::get(const std::string& name) { return (heroes.at(name).get()); }
const Hero& HeroesHandler::getRef(const std::string& name) const { return *get(name); }
Hero& HeroesHandler::getRef(const std::string& name) { return *get(name); }
const Hero& HeroesHandler::operator[](const std::string& name) const { return getRef(name); }
Hero& HeroesHandler::operator[](const std::string& name) { return getRef(name); }
Hero* HeroesHandler::selectedHero() { return paused() ? get(selected) : (Hero*)nullptr; }

bool HeroesHandler::paused() const { return !selected.empty(); }

bool HeroesHandler::isHeroSelected(const std::string& name) const { return name == selected; }

void HeroesHandler::renderUI() {
	raylib::Rectangle heroesRect{158*bgScale, 804*bgScale, 1603*bgScale, 236*bgScale};
	raylib::Rectangle heroRect{heroesRect.x, heroesRect.y, 185*bgScale, heroesRect.height};
	std::vector<int> Ws = {180, 185, 189, 192, 192, 189, 185, 180};
	int spacing = (heroesRect.width - heroRect.width * roster.size()) / (roster.size() - 1);
	for (auto [idx, hero_name] : Utils::enumerate(roster)) {
		auto& hero = getRef(hero_name);
		int i = std::min(idx, (roster.size()-1)-idx);
		heroRect.width = Ws[i] * bgScale;
		hero.renderUI(heroRect);
		heroRect.x += heroRect.width + spacing;
	}

	int points_available = std::accumulate(BEGEND(roster), 0, [&](int tot, const std::string& hero_name){ return tot + getRef(hero_name).skillPoints; });
	if (points_available) {
		auto rect = Utils::anchorRect(detailsTabButton, {15.0f, 15.0f}, Utils::Anchor::topLeft, {}, Utils::AnchorType::center);
		rect.Draw(Dispatch::UI::bgMed);
		rect.DrawLines(BLACK);
		Utils::drawTextAnchored(std::to_string(points_available), rect, Utils::Anchor::center, Dispatch::UI::fontTitle, Dispatch::UI::textColor, 16.0f, 1.0f);
	}

	if (paused()) layoutHeroDetails.render();
}

void updateLayoutStatsData(Dispatch::UI::Layout& layout, const Hero& hero) {
	auto* confirm = layout.get<Dispatch::UI::Button>("stats-confirm");
	auto* reset = layout.get<Dispatch::UI::Button>("stats-reset");
	if (!confirm) throw std::runtime_error("Hero details layout is missing 'stats-confirm' element or it is of the wrong type.");
	if (!reset) throw std::runtime_error("Hero details layout is missing 'stats-reset' element or it is of the wrong type.");

	const auto& attrs = hero.attributes();
	const auto& unc_attrs = hero.unconfirmed_attributes;
	int totalUnconfirmed = 0;

	layout.updateSharedData("skillPoints", std::to_string(hero.skillPoints));
	layout.updateSharedData("attributes", attrs);
	for (Attribute::Value attr : Attribute::Values) {
		std::string attr_str = (std::string)Attribute(attr).toString();
		std::string str_minus = std::format("stats-{}-minus", attr_str);
		std::string str_plus = std::format("stats-{}-plus", attr_str);
		std::string str_value = std::format("stats-{}-value", attr_str);
		auto* minus = layout.get<Dispatch::UI::Button>(str_minus);
		auto* plus = layout.get<Dispatch::UI::Button>(str_plus);
		if (!minus) throw std::runtime_error(std::format("Hero details layout is missing '{}' element or it is of the wrong type.", str_minus));
		if (!plus) throw std::runtime_error(std::format("Hero details layout is missing '{}' element or it is of the wrong type.", str_plus));

		int unconfirmed = unc_attrs[attr];
		totalUnconfirmed += unconfirmed;

		minus->changeStatus(unconfirmed ? Dispatch::UI::Element::Status::REGULAR : Dispatch::UI::Element::Status::DISABLED);
		plus->changeStatus(hero.skillPoints ? Dispatch::UI::Element::Status::REGULAR : Dispatch::UI::Element::Status::DISABLED);
		layout.updateSharedData(str_value, std::to_string(attrs[attr] + unconfirmed));
	}
	auto status = totalUnconfirmed ? Dispatch::UI::Element::Status::REGULAR : Dispatch::UI::Element::Status::DISABLED;
	reset->changeStatus(status);
	confirm->changeStatus(status);
}

bool HeroesHandler::handleInput() {
	auto selectedMission = MissionsHandler::inst().selected;
	auto* mission = selectedMission.empty() ? nullptr : &MissionsHandler::inst()[selectedMission];
	auto missionStatus = selectedMission.empty() ? Mission::DONE : mission->status;
	if (paused()) layoutHeroDetails.handleInput();
	if (raylib::Mouse::IsButtonPressed(MOUSE_BUTTON_LEFT)) {
		raylib::Vector2 mousePos = raylib::Mouse::GetPosition();
		raylib::Rectangle heroesRect{158*bgScale, 804*bgScale, 1603*bgScale, 236*bgScale};
		if (heroesRect.CheckCollision(mousePos)) {
			for (auto [idx, hero_name] : Utils::enumerate(roster)) {
				auto& hero = getRef(hero_name);
				if (hero.uiRect.CheckCollision(mousePos)) {
					if (missionStatus == Mission::SELECTED) {
						if ((hero.status != Hero::AVAILABLE && hero.status != Hero::ASSIGNED) || hero.health == Hero::DOWNED) return false;
						mission->toggleHero(hero_name);
						return true;
					} else if (selectedMission.empty()) {
						selectHero(hero_name);
						return true;
					}
				}
			}
		} else if (detailsTabButton.CheckCollision(mousePos)) {
			if (selected.empty() || !mission->isMenuOpen()) {
				auto it = std::find_if(BEGEND(roster), [&](const std::string& hero_name){ return getRef(hero_name).skillPoints > 0; });
				if (it != roster.end()) selectHero(*it);
				else selectHero(roster[0]);
			}
		}
	}
	if (paused()) {
		auto& hero = getRef(selected);

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
		} else if (layoutHeroDetails.release.contains("backButton")) {
			selectHero("");
			layoutHeroDetails.resetInput();
			return true;
		} else if (layoutHeroDetails.release.contains("stats-reset")) {
			hero.resetAttributeChanges();
			updateLayoutStatsData(layoutHeroDetails, hero);
			return true;
		} else if (layoutHeroDetails.release.contains("stats-confirm")) {
			hero.applyAttributeChanges();
			updateLayoutStatsData(layoutHeroDetails, hero);
			return true;
		} else {
			for (auto attr : Attribute::Values) {
				std::string attr_str = (std::string)Attribute(attr).toString();
				std::string minus = std::format("stats-{}-minus", attr_str);
				std::string plus = std::format("stats-{}-plus", attr_str);
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

void HeroesHandler::update(float deltaTime) {
	for (auto& hero_name : roster) getRef(hero_name).update(deltaTime);
}

void HeroesHandler::selectHero(const std::string& name) {
	selected = name;

	if (paused()) {
		const Hero& hero = getRef(selected);
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
		layoutHeroDetails.updateSharedData("full-image-key", std::format("hero-{}-full", hero.name));
		layoutHeroDetails.updateSharedData("mugshot-image-key", std::format("hero-{}-mugshot", hero.name));
		updateLayoutStatsData(layoutHeroDetails, hero);
	}
}

void HeroesHandler::changeTab(Tab newTab) {
	tab = newTab;
	layoutHeroDetails["stats"]->visible = (tab == UPGRADE);
	layoutHeroDetails["powers"]->visible = (tab == POWERS);
	layoutHeroDetails["info"]->visible = (tab == INFO);
}
