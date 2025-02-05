#pragma once

#include "Utils.h"

/*
* Move representation.
* 'from' and 'to' fields in the square on the board (0-63).
* 'flag' is for additional information, such as for promotions.
*/

class Move
{
public:

	uint8_t from;
	uint8_t to;
	uint8_t flag;

	Move() : Move(0, 0, 0) {}

	Move(const uint8_t from, const uint8_t to) : Move(from, to, 0) {}

	Move(const uint8_t from, const uint8_t to, const uint8_t flag) {
		this->from = from;
		this->to = to;
		this->flag = flag;
	}

	Move(const uint16_t packedMove) {
		from = (packedMove & 0xFC00) >> 10;
		to = (packedMove & 0x03F0) >> 4;
		flag = packedMove & 0x000F;
	}

	std::string ToString(const bool frc) const {

		// Null moves (hopefully you won't see this)
		if (from == 0 && to == 0) return "0000";

		// Castling in standard chess
		if (!frc) {
			if (flag == MoveFlag::ShortCastle) {
				const bool side = from < 32 ? Side::White : Side::Black;
				return side == Side::White ? "e1g1" : "e8g8";
			}
			else if (flag == MoveFlag::LongCastle) {
				const bool side = from < 32 ? Side::White : Side::Black;
				return side == Side::White ? "e1c1" : "e8c8";
			}
		}

		const uint8_t file1 = from % 8;
		const uint8_t rank1 = from / 8;
		const uint8_t file2 = to % 8;
		const uint8_t rank2 = to / 8;

		const char f1 = 'a' + file1;
		const char r1 = '1' + rank1;
		const char f2 = 'a' + file2;
		const char r2 = '1' + rank2;

		const char promo = [&] {
			switch (flag) {
			case MoveFlag::PromotionToQueen: return 'q';
			case MoveFlag::PromotionToRook: return 'r';
			case MoveFlag::PromotionToBishop: return 'b';
			case MoveFlag::PromotionToKnight: return 'n';
			default: return '\0';
			}
		}();

		if (promo == '\0') return { f1, r1, f2, r2 };
		else return { f1, r1, f2, r2, promo };
	}

	inline bool IsEmpty() const {
		return from == 0 && to == 0 && flag == 0;
	}

	inline bool IsNull() const {
		return from == 0 && to == 0;
	}

	inline bool IsUnderpromotion() const {
		return flag == MoveFlag::PromotionToRook || flag == MoveFlag::PromotionToKnight || flag == MoveFlag::PromotionToBishop;
	}

	inline bool IsPromotion() const {
		return flag == MoveFlag::PromotionToQueen || flag == MoveFlag::PromotionToRook
			|| flag == MoveFlag::PromotionToKnight || flag == MoveFlag::PromotionToBishop;
	}

	inline bool IsCastling() const {
		return flag == MoveFlag::ShortCastle || flag == MoveFlag::LongCastle;
	}

	inline uint8_t GetPromotionPieceType() const {
		switch (flag) {
		case MoveFlag::PromotionToQueen: return PieceType::Queen;
		case MoveFlag::PromotionToRook: return PieceType::Rook;
		case MoveFlag::PromotionToKnight: return PieceType::Knight;
		case MoveFlag::PromotionToBishop: return PieceType::Bishop;
		default: return PieceType::None;
		}
	}

	inline uint16_t Pack() const {
		// from: 15..10, to: 9..4, flag: 3..0
		return (from << 10) | (to << 4) | flag;
	}

	inline bool operator== (const Move& m) const {
		return from == m.from && to == m.to && flag == m.flag;
	}

};

static const Move NullMove { 0, 0, MoveFlag::NullMove };
static const Move EmptyMove { 0, 0, MoveFlag::None };

struct MoveAndPiece {
	Move move{};
	uint8_t piece = 0;
};

// Move list --------------------------------------------------------------------------------------

struct ScoredMove {
	Move move;
	int orderScore;
};

struct MoveList : StaticVector<ScoredMove, MaxMoveCount> {

	inline void pushUnscored(const Move& move) {
		assert(count < MaxMoveCount);
		items[count] = { move, 0 };
		count += 1;
	}

	inline void pushScored(const Move& move, const int score) {
		assert(count < MaxMoveCount);
		items[count] = { move, score };
		count += 1;
	}

	inline void setScore(const std::size_t index, const int score) {
		assert(count > index);
		items[index].orderScore = score;
	}

};