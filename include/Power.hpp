#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include <Event.hpp>

class Power {
public:
	Power()=default;
	Power(const nlohmann::json& data, bool unlock=false);

	std::string name, description, hero;
	bool unlocked=false, flight=false;

	template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
	virtual bool preEvent(Event /* event */, EventData& args) {
		return std::visit(overloaded {
			#define GEN_VISIT(T) [this](T& d) { return pre##T(d); },
			EVENT_DATA_LIST(GEN_VISIT)
			#undef GEN_VISIT
			[](auto& /* ignore */) { return true; }
		}, args);
	}
	virtual void onEvent(Event /* event */, EventData& args) {
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
		virtual void on##T(T&) {}
	EVENT_DATA_LIST(GEN_VIRTUALS)
	#undef GEN_VIRTUALS
};

namespace nlohmann {
	inline void to_json(nlohmann::json& j, const Power& inst) { j = nlohmann::json(); inst.to_json(j); }
	inline void from_json(const nlohmann::json& j, Power& inst) { inst.from_json(j); }
}
