#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>

#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>

#include <Utils.hpp>
#include <Event.hpp>
#include <Attribute.hpp>

class Power {
public:
	Power()=default;
	virtual ~Power()=default;

	std::string name, description, hero;
	bool unlocked=false, flight=false, disabled=false;
	std::array<std::set<int>, 4> slotRestriction;

	virtual std::set<Event> getEventList() const;
	virtual bool active() const;
	virtual bool checkSlotRestriction(const EventData& args);

	template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	virtual bool onCheck(Event event, const EventData& args) {
		if (!active()) return true;
		return std::visit(overloaded {
			#define GEN_VISIT(NAME, DATA) [this, event](const DATA& d) { \
				if (event == Event::NAME) return check##NAME(event, d);  \
				else return checkAny##NAME(event, d);                    \
			},
			BASE_EVENT_LIST(GEN_VISIT)
			#undef GEN_VISIT
			[](auto& /* ignore */) { return true; }
		}, args);
	}
	virtual void onEvent(Event event, const EventData& args) {
		if (!unlocked) return;
		std::visit(overloaded {
			#define GEN_VISIT(NAME, DATA) [this, event](const DATA& d) { \
				if (event == Event::NAME) on##NAME(event, d);            \
				else onAny##NAME(event, d);                              \
			},
			BASE_EVENT_LIST(GEN_VISIT)
			#undef GEN_VISIT
			[](const auto& /* ignore */) {}
		}, args);
	}

	virtual void to_json(nlohmann::json& j) const;
	virtual void from_json(const nlohmann::json& j);

	static std::unique_ptr<Power> power_factory(const std::string& type);
	static std::unique_ptr<Power> power_factory(const nlohmann::json& data);
protected:
	#define GEN_VIRTUALS(NAME, DATA) \
		virtual bool check##NAME(Event, const DATA&) { return true; } \
		virtual bool checkAny##NAME(Event, const DATA&) { return true; } \
		virtual void on##NAME(Event, const DATA&) { Utils::println("Event {} called, for power {} of hero {}", #NAME, name, hero); } \
		virtual void onAny##NAME(Event, const DATA&) { Utils::println("Event Any{} called, for power {} of hero {}", #NAME, name, hero); }
	BASE_EVENT_LIST(GEN_VIRTUALS)
	#undef GEN_VIRTUALS
};


class AttrBonusPower : public virtual Power {
public:
	enum Operator {
		PLUS,
		MINUS,
		MULTIPLY,
		DIVIDE,
		ASSIGN
	};
	struct Operation {
		Attribute attribute{Attribute::COMBAT};
		Operator oper{PLUS};
		int value=1;
	};
	std::map<Event, std::vector<Operation>> operations;
	AttrMap<int> bonus;
	int lowerLimit=0, upperLimit=10;
	enum AppliesTo {
		SELF,
		OTHERS,
		LEFT,
		RIGHT,
		ALL,
		ALL_LEFT,
		ALL_RIGHT
	} appliesTo = SELF;

	virtual std::set<Event> getEventList() const override;

	virtual void onEvent(Event event, const EventData& args) override;
	virtual void onHeroCalcAttr(Event event, const HeroCalcAttrData&) override;
	virtual void onAnyHeroCalcAttr(Event event, const HeroCalcAttrData&) override;
	virtual void onMissionStart(Event event, const MissionStartData&) override;
	virtual void onMissionSuccess(Event event, const MissionSuccessData&) override;
	virtual void onMissionFailure(Event event, const MissionFailureData&) override;

	void onMission(const std::vector<std::string>& assignedSlots);
	bool applies(int mySlot, int otherSlot);

	virtual void to_json(nlohmann::json& j) const override;
	virtual void from_json(const nlohmann::json& j) override;
};

void to_json(nlohmann::json& j, const ::AttrBonusPower::Operation& inst);
void from_json(const nlohmann::json& j, ::AttrBonusPower::Operation& inst);

NLOHMANN_JSON_SERIALIZE_ENUM( ::AttrBonusPower::AppliesTo, {
	{ ::AttrBonusPower::AppliesTo::SELF, "SELF"},
	{ ::AttrBonusPower::AppliesTo::OTHERS, "OTHERS"},
	{ ::AttrBonusPower::AppliesTo::LEFT, "LEFT"},
	{ ::AttrBonusPower::AppliesTo::RIGHT, "RIGHT"},
	{ ::AttrBonusPower::AppliesTo::ALL_LEFT, "ALL_LEFT"},
	{ ::AttrBonusPower::AppliesTo::ALL_RIGHT, "ALL_RIGHT"},
});
NLOHMANN_JSON_SERIALIZE_ENUM( ::AttrBonusPower::Operator, {
	{ ::AttrBonusPower::Operator::PLUS, "+" },
	{ ::AttrBonusPower::Operator::MINUS, "-" },
	{ ::AttrBonusPower::Operator::MULTIPLY, "*" },
	{ ::AttrBonusPower::Operator::DIVIDE, "/" },
	{ ::AttrBonusPower::Operator::ASSIGN, "=" },
});

inline void to_json(nlohmann::json& j, const ::Power& inst) { j = nlohmann::json(); inst.to_json(j); }
inline void from_json(const nlohmann::json& j, ::Power& inst) { inst.from_json(j); }
inline void to_json(nlohmann::json& j, const ::AttrBonusPower& inst) { j = nlohmann::json(); inst.to_json(j); }
inline void from_json(const nlohmann::json& j, ::AttrBonusPower& inst) { inst.from_json(j); }
