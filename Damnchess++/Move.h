#pragma once

#include <string>
#include "Utils.cpp"

class Move
{
public:
	Move(int from, int to);
	Move(int from, int to, int flag);
	void SetFlag(int flag);
	std::string ToString();

	int from;
	int to;
	int flag;
};

