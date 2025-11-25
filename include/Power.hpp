#pragma once

#include <string>
#include <JSONish.hpp>

class Power {
public:
	Power(const JSONish::Node& data, bool unlock=false);

	std::string name, description;
	bool unlocked=false, flight=false;
};
