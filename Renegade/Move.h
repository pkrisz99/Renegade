#pragma once

#include <string>
#include "Utils.h"

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
	const std::string ToString() const;
	const bool IsNotNull();
	const bool IsEmpty();
	const bool IsPromotion() const;
	const bool IsUnderpromotion();
	const uint8_t GetPromotionPieceType() const;
	const bool operator== (const Move& m) const;

	uint8_t from;
	uint8_t to;
	uint8_t flag;

};

static const Move NullMove { 0, 0, MoveFlag::NullMove };
static const Move EmptyMove { 0, 0, MoveFlag::None };