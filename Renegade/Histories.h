#pragma once
#include "Move.h"
#include "Position.h"
#include "Utils.h"
#include <memory>

// Aliases for readability
constexpr bool Bonus = true;
constexpr bool Penalty = false;

class Histories
{
public:
	
	Histories();
	void ClearAll();
	void ClearRefutations();

	// Refutation moves:
	void SetKillerMove(const Move& move, const int level);
	void SetCountermove(const Move& previousMove, const Move& thisMove);
	void SetPositionalMove(const Position& pos, const Move& thisMove);
	std::tuple<Move, Move, Move> GetRefutationMoves(const Position& pos, const int level) const;
	void ResetKillerForPly(const int level);

	// History heuristic:
	template <bool bonus> void UpdateQuietHistory(const Position& position, const Move& m, const int level, const int depth, const int times);
	template <bool bonus> void UpdateCaptureHistory(const Position& position, const Move& m, const int depth, const int times);
	int GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const;
	int GetCaptureHistoryScore(const Position& position, const Move& m) const;

	// Correction history for position evaluations:
	void UpdateCorrection(const Position& position, const int16_t refEval, const int16_t score, const int depth);
	int16_t ApplyCorrection(const Position& position, const int16_t rawEval) const;

private:

	inline void UpdateHistoryValue(int16_t& value, const int amount) {
		const int gravity = value * std::abs(amount) / 16600;
		value += amount - gravity;
	}

	inline void UpdateHistoryValueCustomGravity(int16_t& value, int16_t gravityBase, const int amount) {
		const int gravity = gravityBase * std::abs(amount) / 16600;
		const int newValue = std::clamp(value + amount - gravity, -16600, 16600);
		value = static_cast<int16_t>(newValue);
	}

	// Refutations:
	std::array<Move, MaxDepth> KillerMoves;
	MultiArray<Move, 64, 64> CounterMoves;
	MultiArray<Move, 2, 8192> PositionalMoves;

	// Move ordering history:
	MultiArray<int16_t, 15, 64, 2, 2> QuietHistory;
	MultiArray<int16_t, 15, 64, 15, 2, 2> CaptureHistory; // [attacking piece][square to][captured piece][from treat][to threat]
	MultiArray<int16_t, 15, 64, 15, 64> ContinuationHistory;

	// Evaluation correction history:
	MultiArray<int32_t, 2, 16384> PawnCorrectionHistory;
	MultiArray<int32_t, 2, 2, 65536> NonPawnCorrectionHistory;
	MultiArray<int32_t, 15, 64, 15, 64> FollowUpCorrectionHistory;
};

