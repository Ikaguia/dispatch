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
	bool unlocked=false, flight=false;

	virtual std::set<Event> getEventList() const;

	template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	virtual bool preEvent(Event /* event */, EventData& args) {
		if (!unlocked) return true;
		return std::visit(overloaded {
			#define GEN_VISIT(T) [this](T& d) { return pre##T(d); },
			EVENT_DATA_LIST(GEN_VISIT)
			#undef GEN_VISIT
			[](auto& /* ignore */) { return true; }
		}, args);
	}
	virtual void onEvent(Event /* event */, EventData& args) {
		if (!unlocked) return;
		std::visit(overloaded {
			#define GEN_VISIT(T) [this](T& d) { on##T(d); },
			EVENT_DATA_LIST(GEN_VISIT)
			#undef GEN_VISIT
			[](auto& /* ignore */) {}
		}, args);
	}

	virtual void to_json(nlohmann::json& j) const;
	virtual void from_json(const nlohmann::json& j);

	static std::unique_ptr<Power> power_factory(const std::string& type);
	static std::unique_ptr<Power> power_factory(const nlohmann::json& data);
protected:
	#define GEN_VIRTUALS(T) \
		virtual bool pre##T(T&) { return true; } \
		virtual void on##T(T&) { Utils::println("Event {} called, for power {} of hero {}", #T, name, hero); }
	EVENT_DATA_LIST(GEN_VIRTUALS)
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

	virtual std::set<Event> getEventList() const override;

	virtual void onEvent(Event event, EventData& args) override;
	virtual bool preHeroCalcAttrData(HeroCalcAttrData&) override;
	virtual void onHeroCalcAttrData(HeroCalcAttrData&) override;

	virtual void to_json(nlohmann::json& j) const override;
	virtual void from_json(const nlohmann::json& j) override;
};

void to_json(nlohmann::json& j, const ::AttrBonusPower::Operation& inst);
void from_json(const nlohmann::json& j, ::AttrBonusPower::Operation& inst);

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
