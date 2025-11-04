#include <Hero.hpp>
#include <Utils.hpp>
#include <cctype>

Hero::Hero(const std::string& name, const std::map<std::string, int> &attr) : name(name), attributes{} {
	for (auto [attrName, value] : attr) {
		Attribute attribute = Attribute::fromString(attrName);
		attributes[attribute] = value;
	}
};
