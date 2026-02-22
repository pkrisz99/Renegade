#pragma once
#include "Histories.h"
#include "Position.h"
#include "Utils.h"
#include <algorithm>
#include <array>


enum class MovePickerStage {
	EmitTTMove = 0,
	GenerateAndScoreNoisyMoves,
	EmitGoodNoisyMoves,
	GenerateAndScoreQuietMoves,
	EmitQuietMoves,
	EmitBadNoisyMoves,
	End
};


class MovePicker {
public:
	MovePicker() = default;

	// Sets up the move generation
	// This is not a constructor as reallocation seems to negatively affect performance.
	void initialize(const bool skipQuietMoves, const Position& pos, const Histories& hist, const Move& ttMove, const int level) {
		this->stage = MovePickerStage::EmitTTMove;
		this->ttMove = ttMove;
		std::tie(killerMove, counterMove, positionalMove) = hist.GetRefutationMoves(pos, level);
		this->level = level;
		this->skipQuietMoves = skipQuietMoves;
		this->noisyMoveIndex = 0;
		this->quietMoveIndex = 0;
		this->noisyMoves.clear();
		this->quietMoves.clear();
	}

	// Selects the next move (and also returns the order value)
	// The order is based on information about the position and prior knowledge the engine keeps track of, from most
	// to least promising. This logic ensures to lazily do the computations we might not need (known as staged movegen).
	std::pair<Move, int> next(const Position& pos, const Histories& hist) {

		switch (stage) {

		case MovePickerStage::EmitTTMove:
			stage = MovePickerStage::GenerateAndScoreNoisyMoves;
			if (!ttMove.IsNull() && pos.IsPseudoLegalMove(ttMove) && pos.IsLegalMove(ttMove)) {
				return { ttMove, 900000 };
			}
			[[fallthrough]];

		case MovePickerStage::GenerateAndScoreNoisyMoves:
			pos.GenerateNoisyPseudoLegalMoves(noisyMoves);
			for (auto& m : noisyMoves) m.orderScore = getNoisyMoveScore(pos, hist, m.move);
			stage = MovePickerStage::EmitGoodNoisyMoves;
			[[fallthrough]];

		case MovePickerStage::EmitGoodNoisyMoves:
			while (noisyMoveIndex < noisyMoves.size()) {
				const auto next = findNext(noisyMoves, noisyMoveIndex);
				if (next.first == ttMove) continue;
				if (!pos.IsLegalMove(next.first)) continue;
				if (next.second < -100000) {
					noisyMoveIndex -= 1;
					break;
				}
				return next;
			}
			stage = MovePickerStage::GenerateAndScoreQuietMoves;
			[[fallthrough]];

		case MovePickerStage::GenerateAndScoreQuietMoves:
			if (!skipQuietMoves) {
				pos.GenerateQuietPseudoLegalMoves(quietMoves);
				for (auto& m : quietMoves) m.orderScore = getQuietMoveScore(pos, hist, m.move);
				stage = MovePickerStage::EmitQuietMoves;
			}
			[[fallthrough]];

		case MovePickerStage::EmitQuietMoves:
			if (!skipQuietMoves) {
				while (quietMoveIndex < quietMoves.size()) {
					const auto next = findNext(quietMoves, quietMoveIndex);
					if (next.first == ttMove) continue;
					if (!pos.IsLegalMove(next.first)) continue;
					return next;
				}
			}
			stage = MovePickerStage::EmitBadNoisyMoves;
			[[fallthrough]];

		case MovePickerStage::EmitBadNoisyMoves:
			while (noisyMoveIndex < noisyMoves.size()) {
				const auto next = findNext(noisyMoves, noisyMoveIndex);
				if (next.first == ttMove) continue;
				if (!pos.IsLegalMove(next.first)) continue;
				return next;
			}
			stage = MovePickerStage::End;
			[[fallthrough]];

		case MovePickerStage::End:
			return { NullMove, -999999 };
		}
	}

	static constexpr int MaxRegularQuietOrder = 200000;
	bool skipQuietMoves = false;

private:

	// Selects the move with the next highest score in the list
	std::pair<Move, int> findNext(MoveList& moves, size_t& index) const {
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

	// Score quiet moves: take the history score and potentially apply a bonus for being a refutation
	int getQuietMoveScore(const Position& pos, const Histories& hist, const Move& m) const {

		const uint8_t movedPiece = pos.GetPieceAt(m.from);
		int historyScore = hist.GetHistoryScore(pos, m, movedPiece, level);

		if (m == killerMove) historyScore += 17700;
		else if (m == positionalMove) historyScore += 16300;
		else if (m == counterMove) historyScore += 14000;

		return historyScore;
	}

	// Score noisy moves: take history, piece types and static exchange eval into account
	int getNoisyMoveScore(const Position& pos, const Histories& hist, const Move& m) const {

		constexpr std::array<int, 7> values = { 0, 100, 300, 300, 500, 900, 0 };
		const uint8_t capturedPieceType = [&] {
			if (m.flag == MoveFlag::EnPassantPerformed) return PieceType::Pawn;
			return TypeOfPiece(pos.GetPieceAt(m.to));
		}();

		// Queen promotions
		if (m.flag == MoveFlag::PromotionToQueen) return 700000 + values[capturedPieceType];

		const bool losingCapture = [&] {
			if (skipQuietMoves) return false;
			const int16_t captureScore = (m.IsPromotion()) ? 0 : hist.GetCaptureHistoryScore(pos, m);
			return !pos.StaticExchangeEval(m, -captureScore / 28);
		}();
		return (!losingCapture ? 600000 : -200000) + values[capturedPieceType] * 18 + hist.GetCaptureHistoryScore(pos, m);
	}

	size_t noisyMoveIndex = 0, quietMoveIndex = 0;
	Move ttMove{}, killerMove{}, counterMove{}, positionalMove{};
	MoveList noisyMoves{}, quietMoves{};
	int level = 0;
	MovePickerStage stage = MovePickerStage::EmitTTMove;
};