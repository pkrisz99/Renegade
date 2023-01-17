#pragma once

#include <string>
#include "Utils.cpp"

class Move
{
public:
	Move();
	Move(int from, int to);
	Move(int from, int to, int flag);
	void SetFlag(int flag);
	const std::string ToString();
	const bool IsNotNull();
	const bool IsEmpty();
	const bool IsUnderpromotion();

	int from;
	int to;
	int flag;
};

static bool operator == (const Move &m1, const Move &m2) {
	return (m1.from == m2.from) && (m1.to == m2.to) && (m1.flag == m2.flag);
}