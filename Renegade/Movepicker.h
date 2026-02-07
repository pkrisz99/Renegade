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

	MovePicker(const bool onlyNoisyMoves, const Position& pos, const Histories& hist, const Move& ttMove, const int level) {
		this->stage = MovePickerStage::EmitTTMove;
		this->ttMove = ttMove;
		std::tie(killerMove, counterMove, positionalMove) = hist.GetRefutationMoves(pos, level);
		this->level = level;
		this->onlyNoisyMoves = onlyNoisyMoves;
		this->noisyMoveIndex = 0;
		this->quietMoveIndex = 0;
	}

	// Retrieves the next move
	std::pair<Move, int> Next(const Position& pos, const Histories& hist) {

		switch (stage) {

		case MovePickerStage::EmitTTMove:
			// We can sometimes skip the entire move generation process
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
			if (onlyNoisyMoves) {
				stage = MovePickerStage::EmitBadNoisyMoves;
				goto EmitBadNoisyMoves; // horrible
			}
			stage = MovePickerStage::GenerateAndScoreQuietMoves;
			[[fallthrough]];

		case MovePickerStage::GenerateAndScoreQuietMoves:
			pos.GenerateQuietPseudoLegalMoves(quietMoves);
			for (auto& m : quietMoves) m.orderScore = getQuietMoveScore(pos, hist, m.move);
			stage = MovePickerStage::EmitQuietMoves;
			[[fallthrough]];

		case MovePickerStage::EmitQuietMoves:
			while (quietMoveIndex < quietMoves.size()) {
				const auto next = findNext(quietMoves, quietMoveIndex);
				if (next.first == ttMove) continue;
				if (!pos.IsLegalMove(next.first)) continue;
				return next;
			}
			stage = MovePickerStage::EmitBadNoisyMoves;

		EmitBadNoisyMoves:
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

private:

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

	int getQuietMoveScore(const Position& pos, const Histories& hist, const Move& m) const {

		if (m == ttMove) return 900000;

		// Quiet moves: take the history score and potentially apply a bonus for being a refutation
		const uint8_t movedPiece = pos.GetPieceAt(m.from);
		int historyScore = hist.GetHistoryScore(pos, m, movedPiece, level);

		if (m == killerMove) historyScore += 17700;
		else if (m == positionalMove) historyScore += 16300;
		else if (m == counterMove) historyScore += 14000;

		return historyScore;
	}

	int getNoisyMoveScore(const Position& pos, const Histories& hist, const Move& m) const {

		if (m == ttMove) return 900000;

		constexpr std::array<int, 7> values = { 0, 100, 300, 300, 500, 900, 0 };
		const uint8_t capturedPieceType = [&] {
			if (m.flag == MoveFlag::EnPassantPerformed) return PieceType::Pawn;
			return TypeOfPiece(pos.GetPieceAt(m.to));
		}();

		// Queen promotions
		if (m.flag == MoveFlag::PromotionToQueen) return 700000 + values[capturedPieceType];

		// Captures
		const bool losingCapture = [&] {
			if (onlyNoisyMoves) return false;
			const int16_t captureScore = (m.IsPromotion()) ? 0 : hist.GetCaptureHistoryScore(pos, m);
			return !pos.StaticExchangeEval(m, -captureScore / 28);
		}();
		return (!losingCapture ? 600000 : -200000) + values[capturedPieceType] * 18 + hist.GetCaptureHistoryScore(pos, m);
	}

	size_t noisyMoveIndex = 0, quietMoveIndex = 0;
	Move ttMove{}, killerMove{}, counterMove{}, positionalMove{};
	MoveList noisyMoves{}, quietMoves{};
	int level = 0;
	bool onlyNoisyMoves = false;
	MovePickerStage stage = MovePickerStage::EmitTTMove;
};
