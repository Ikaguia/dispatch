#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

#include <nlohmann/json.hpp>
#include <nlohmann/detail/macro_scope.hpp>

#include <Attribute.hpp>
#include <Event.hpp>

class Hero; class Power;

class Effect : public std::enable_shared_from_this<Effect> {
	static std::shared_ptr<Effect> effect_factory(const std::string& type);
public:
	Effect()=default;
	virtual ~Effect()=default;

	Hero* hero;
	Power* power;
	bool disabled=true;
	std::array<std::set<int>, 4> slotRestriction;

	static std::shared_ptr<Effect> effect_factory(const nlohmann::json& data, Hero* h, Power* p);

	virtual std::set<Event> getEventList() const;
	virtual bool active() const;
	virtual bool checkSlotRestriction(const EventData& args);

	virtual bool onCheck(Event event, const EventData& args);
	virtual void onEvent(Event event, const EventData& args);

	virtual void to_json(nlohmann::json& j) const;
	virtual void from_json(const nlohmann::json& j);
protected:
	#define GEN_VIRTUALS(NAME, DATA) \
		virtual bool check##NAME(Event, const DATA&); \
		virtual bool checkAny##NAME(Event, const DATA&); \
		virtual void on##NAME(Event, const DATA&); \
		virtual void onAny##NAME(Event, const DATA&);
	BASE_EVENT_LIST(GEN_VIRTUALS)
	#undef GEN_VIRTUALS
};


class AttrBonusEffect : public virtual Effect {
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

void to_json(nlohmann::json& j, const ::AttrBonusEffect::Operation& inst);
void from_json(const nlohmann::json& j, ::AttrBonusEffect::Operation& inst);

NLOHMANN_JSON_SERIALIZE_ENUM( ::AttrBonusEffect::AppliesTo, {
	{ ::AttrBonusEffect::AppliesTo::SELF, "SELF"},
	{ ::AttrBonusEffect::AppliesTo::OTHERS, "OTHERS"},
	{ ::AttrBonusEffect::AppliesTo::LEFT, "LEFT"},
	{ ::AttrBonusEffect::AppliesTo::RIGHT, "RIGHT"},
	{ ::AttrBonusEffect::AppliesTo::ALL_LEFT, "ALL_LEFT"},
	{ ::AttrBonusEffect::AppliesTo::ALL_RIGHT, "ALL_RIGHT"},
});
NLOHMANN_JSON_SERIALIZE_ENUM( ::AttrBonusEffect::Operator, {
	{ ::AttrBonusEffect::Operator::PLUS, "+" },
	{ ::AttrBonusEffect::Operator::MINUS, "-" },
	{ ::AttrBonusEffect::Operator::MULTIPLY, "*" },
	{ ::AttrBonusEffect::Operator::DIVIDE, "/" },
	{ ::AttrBonusEffect::Operator::ASSIGN, "=" },
});

inline void to_json(nlohmann::json& j, const Effect& inst) { j = nlohmann::json(); inst.to_json(j); }
inline void from_json(const nlohmann::json& j, Effect& inst) { inst.from_json(j); }
inline void to_json(nlohmann::json& j, const AttrBonusEffect& inst) { j = nlohmann::json(); inst.to_json(j); }
inline void from_json(const nlohmann::json& j, AttrBonusEffect& inst) { inst.from_json(j); }
