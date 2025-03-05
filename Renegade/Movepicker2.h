#pragma once
#include "Histories.h"
#include "Position.h"
#include "Utils.h"
#include <algorithm>
#include <array>
//#include <memory>

template <MoveGen movegen>
class MovePicker {
public:
	MovePicker(const Position& pos, const Histories& hist, const Move& ttMove, const Move& killerMove, const Move& counterMove,
		const int level): pos(pos), hist(hist) {
		this->ttMove = ttMove;
		this->killerMove = killerMove;
		this->counterMove = counterMove;
		this->level = level;
		index = 0;

		pos.GenerateMoves(moves, movegen, Legality::Pseudolegal);
		for (auto& m : moves) m.orderScore = GetMoveScore(m.move);
	}

	std::pair<Move, int> Get() {
		assert(hasNext());

		int bestOrderScore = moves[index].orderScore;
		int bestIndex = index;

		for (int i = index + 1; i < moves.size(); i++) {
			if (moves[i].orderScore > bestOrderScore) {
				bestOrderScore = moves[i].orderScore;
				bestIndex = i;
			}
		}

		std::swap(moves[bestIndex], moves[index]);
		index += 1;
		return { moves[index - 1].move, moves[index - 1].orderScore };
	}

	bool HasNext() const {
		return index < moves.size();
	}

private:

	int GetMoveScore(const Move& m) const {

		// Transposition move
		if (m == ttMove) return 900000;

		constexpr std::array<int, 7> values = { 0, 100, 300, 300, 500, 900, 0 };
		const uint8_t movedPiece = pos.GetPieceAt(m.from);
		const uint8_t attackingPieceType = TypeOfPiece(movedPiece);
		const uint8_t capturedPieceType = [&] {
			if (m.IsCastling()) return PieceType::None;
			if (m.flag == MoveFlag::EnPassantPerformed) return PieceType::Pawn;
			return TypeOfPiece(pos.GetPieceAt(m.to));
			}();

		// Queen promotions
		if (m.flag == MoveFlag::PromotionToQueen) return 700000 + values[capturedPieceType];

		// Captures
		if (capturedPieceType != PieceType::None) {

			const bool losingCapture = [&] {
				if (movegen	== MoveGen::Noisy) return false;
				if (pos.IsMoveQuiet(m)) return false;
				const int16_t captureScore = (m.IsPromotion()) ? 0 : hist.GetCaptureHistoryScore(pos, m);
				return !pos.StaticExchangeEval(m, -captureScore / 32);
			}();

			if (!losingCapture) return 600000 + values[capturedPieceType] * 16 + hist.GetCaptureHistoryScore(pos, m);
			else return -200000 + values[capturedPieceType] * 16 + hist.GetCaptureHistoryScore(pos, m);
		}

		// Quiet killer moves
		if (m == killerMove) return 100000;

		// Countermove heuristic
		if (m == counterMove) return 99000;

		// Quiet moves
		const int historyScore = hist.GetHistoryScore(pos, m, movedPiece, level);
		return historyScore;
	}


	Move ttMove, killerMove, counterMove;
	MoveList moves{};
	int level;
	int index;
	const Position& pos;
	const Histories& hist;
};