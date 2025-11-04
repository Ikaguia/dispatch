#pragma once

#include <string>
#include <map>
#include <vector>

#include <Attribute.hpp>

class Hero {
public:
	std::string name;
	AttrMap<int> attributes;

	Hero(const std::string& name, const std::map<std::string,int> &attr = {});
};
