#pragma once

#include "Globals.hpp"

class timefmt
{
public:
	timefmt(std::string fmt)
		: format(fmt) { }

	friend std::ostream& operator <<(std::ostream &, timefmt const &);

private:
	std::string format;
};