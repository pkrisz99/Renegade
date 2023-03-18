#pragma once

#include <string>
#include "Utils.cpp"

/*
* Move representation.
* 'from' and 'to' fields in the square on the board (0-63).
* 'flag' is for additional information, such as for promotions.
*/

class Move
{
public:
	Move();
	Move(uint8_t from, uint8_t to);
	Move(uint8_t from, uint8_t to, uint8_t flag);
	void SetFlag(uint8_t flag);
	const std::string ToString();
	const bool IsNotNull();
	const bool IsEmpty();
	const bool IsUnderpromotion();

	uint8_t from;
	uint8_t to;
	uint8_t flag;
};

static bool operator == (const Move &m1, const Move &m2) {
	return (m1.from == m2.from) && (m1.to == m2.to) && (m1.flag == m2.flag);
}