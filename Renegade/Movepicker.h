#pragma once
#include "Histories.h"
#include "Position.h"
#include "Utils.h"
#include <algorithm>
#include <array>


enum class MovePickerStage {
	EmitTTMove,
	GenerateAndScoreMoves,
	EmitMoves,
	End
};


class MovePicker {
public:
	MovePicker() = delete;

	MovePicker(const MoveGen moveGen, const Position& pos, const Histories& hist, const Move& ttMove, const int level) {
		this->stage = MovePickerStage::EmitTTMove;
		this->ttMove = ttMove;
		std::tie(killerMove, counterMove, positionalMove) = hist.GetRefutationMoves(pos, level);
		this->level = level;
		this->moveGen = moveGen;
		this->index = 0;
	}

	// Retrieves the next move
	std::pair<Move, int> Next(const Position& pos, const Histories& hist) {

		switch (stage) {

		// 1. Return the TT move if possible, skipping move generation entirely
		case MovePickerStage::EmitTTMove:
			stage = MovePickerStage::GenerateAndScoreMoves;
			if (!ttMove.IsNull() && pos.IsPseudoLegalMove(ttMove) && pos.IsLegalMove(ttMove)) {
				return { ttMove, 900000 };
			}
			[[fallthrough]];

		// 2. Generate moves and score them
		case MovePickerStage::GenerateAndScoreMoves:
			pos.GenerateMoves(moves, moveGen, Legality::Pseudolegal);
			for (auto& m : moves) m.orderScore = GetMoveScore(pos, hist, m.move);
			stage = MovePickerStage::EmitMoves;
			[[fallthrough]];

		// 3. Return moves in decreasing order of merit
		case MovePickerStage::EmitMoves:
			while (index < moves.size()) {
				const auto next = FindNext();
				if (next.first == ttMove) continue;
				if (!pos.IsLegalMove(next.first)) continue;
				return next;
			}
			stage = MovePickerStage::End;
			[[fallthrough]];

		// 4. Signal that all legal moves have been emitted
		case MovePickerStage::End:
			return { NullMove, -999999 };
		
		}
	}

	static constexpr int MaxRegularQuietOrder = 200000;

private:

	std::pair<Move, int> FindNext() {
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
				if (onlyNoisyMoves) return false;
				if (pos.IsMoveQuiet(m)) return false;
				const int16_t captureScore = (m.IsPromotion()) ? 0 : hist.GetCaptureHistoryScore(pos, m);
				return !pos.StaticExchangeEval(m, -captureScore / 28);
			}();
			return (!losingCapture ? 600000 : -200000) + values[capturedPieceType] * 18 + hist.GetCaptureHistoryScore(pos, m);
		}

		// Quiet moves: take the history score and potentially apply a bonus for being a refutation
		int historyScore = hist.GetHistoryScore(pos, m, movedPiece, level);

		if (m == killerMove) historyScore += 17700;
		else if (m == positionalMove) historyScore += 16300;
		else if (m == counterMove) historyScore += 14000;

		return historyScore;
	}


	size_t index = 0;
	Move ttMove{}, killerMove{}, counterMove{}, positionalMove{};
	MoveList moves{};
	int level = 0;
	MoveGen moveGen = MoveGen::All;
	MovePickerStage stage = MovePickerStage::EmitTTMove;
};