#pragma once
#include "Histories.h"
#include "Position.h"
#include "Utils.h"
#include <algorithm>
#include <array>

class MovePicker {
public:
	MovePicker() = default;

	void Initialize(const MoveGen moveGen, const Position& pos, const Histories& hist, const Move& ttMove, const int level) {
		this->ttMove = ttMove;
		std::tie(killerMove, counterMove, positionalMove) = hist.GetRefutationMoves(pos, level);
		this->level = level;
		this->moveGen = moveGen;
		this->moves.clear();
		this->index = 0;

		pos.GenerateMoves(moves, moveGen, Legality::Pseudolegal);
		for (auto& m : moves) m.orderScore = GetMoveScore(pos, hist, m.move);
	}

	std::pair<Move, int> Get() {
		assert(HasNext());

		int bestOrderScore = moves[index].orderScore;
		int bestIndex = index;

		for (int i = index + 1; i < static_cast<int>(moves.size()); i++) {
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
		return index < static_cast<int>(moves.size());
	}

	int index = 0;

	static constexpr int MaxRegularQuietOrder = 200000;

private:

	int GetMoveScore(const Position& pos, const Histories& hist, const Move& m) const {

		// Transposition move
		if (m == ttMove) return 900000;

		constexpr std::array<int, 7> values = { 0, 100, 300, 300, 500, 900, 0 };
		const uint8_t movedPiece = pos.GetPieceAt(m.from);
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
				if (moveGen == MoveGen::Noisy) return false;
				if (pos.IsMoveQuiet(m)) return false;
				const int16_t captureScore = (m.IsPromotion()) ? 0 : hist.GetCaptureHistoryScore(pos, m);
				return !pos.StaticExchangeEval(m, -captureScore / 33);
			}();
			return (!losingCapture ? 600000 : -200000) + values[capturedPieceType] * 16 + hist.GetCaptureHistoryScore(pos, m);
		}

		// Quiet moves: take the history score and potentially apply a bonus for being a refutation
		int historyScore = hist.GetHistoryScore(pos, m, movedPiece, level);

		if (m == killerMove) historyScore += 22000;
		else if (m == counterMove) historyScore += 16000;
		else if (m == positionalMove) historyScore += 16000;

		return historyScore;
	}


	Move ttMove{}, killerMove{}, counterMove{}, positionalMove{};
	MoveList moves{};
	int level = 0;
	MoveGen moveGen = MoveGen::All;
};