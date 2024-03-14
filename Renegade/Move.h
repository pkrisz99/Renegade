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

	Move(uint8_t from, uint8_t to) : Move(from, to, 0) {}

	Move(uint8_t from, uint8_t to, uint8_t flag) {
		this->from = from;
		this->to = to;
		this->flag = flag;
	}

	std::string ToString() const {
		if (from == 0 && to == 0) return "0000";

		const int file1 = from % 8;
		const int rank1 = from / 8;
		const int file2 = to % 8;
		const int rank2 = to / 8;

		const char f1 = 'a' + file1;
		const char r1 = '1' + rank1;
		const char f2 = 'a' + file2;
		const char r2 = '1' + rank2;

		char extra = '?';
		switch (flag) {
		case MoveFlag::PromotionToKnight: extra = 'n'; break;
		case MoveFlag::PromotionToBishop: extra = 'b'; break;
		case MoveFlag::PromotionToRook: extra = 'r'; break;
		case MoveFlag::PromotionToQueen: extra = 'q'; break;
		}

		if (extra == '?') return { f1, r1, f2, r2 };
		else return { f1, r1, f2, r2, extra };
	}

	inline bool IsEmpty() const {
		return (from == 0) && (to == 0) && (flag == 0);
	}

	inline bool IsNull() const {
		return (from == 0) && (to == 0);
	}

	inline bool IsNotNull() const {
		return (from != 0) || (to != 0);
	}

	inline bool IsUnderpromotion() const {
		return (flag == MoveFlag::PromotionToRook) || (flag == MoveFlag::PromotionToKnight) || (flag == MoveFlag::PromotionToBishop);
	}

	inline bool IsPromotion() const {
		return (flag == MoveFlag::PromotionToQueen) || (flag == MoveFlag::PromotionToRook)
			|| (flag == MoveFlag::PromotionToKnight) || (flag == MoveFlag::PromotionToBishop);
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

	inline bool operator== (const Move& m) const {
		return (from == m.from) && (to == m.to) && (flag == m.flag);
	}

};

static const Move NullMove { 0, 0, MoveFlag::NullMove };
static const Move EmptyMove { 0, 0, MoveFlag::None };

struct MoveAndPiece {
	Move move;
	uint8_t piece = 0;
};

// Move list --------------------------------------------------------------------------------------

constexpr int MaxMoveCount = 256;

struct ScoredMove {
	Move move;
	int orderScore;
};

struct MoveList {
	std::array<ScoredMove, MaxMoveCount> moves{};
	int count = 0;

	inline void pushUnscored(const Move& move) {
		assert(count < MaxMoveCount);
		moves[count] = { move, 0 };
		count += 1;
	}

	inline void pushScored(const Move& move, const int score) {
		assert(count < MaxMoveCount);
		moves[count] = { move, score };
		count += 1;
	}

	inline ScoredMove& operator[](const std::size_t index) {
		return moves[index];
	}

	inline void setScore(const std::size_t index, const int score) {
		assert(count > index);
		moves[index].orderScore = score;
	}

	inline auto begin() const {
		return moves.begin();
	}

	inline auto end() const {
		return moves.begin() + static_cast<std::ptrdiff_t>(count);
	}

	inline auto begin() {
		return moves.begin();
	}

	inline auto end() {
		return moves.begin() + static_cast<std::ptrdiff_t>(count);
	}

	inline std::size_t size() const {
		return count;
	}

	inline void reset() {
		count = 0;
	}

	std::vector<Move> vectorize() const {
		std::vector<Move> v{};
		v.reserve(count);
		for (int i = 0; i < count; i++) v.push_back(moves[i].move);
		return v;
	}
};