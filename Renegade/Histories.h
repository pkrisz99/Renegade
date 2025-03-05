#pragma once
#include "Move.h"
#include "Position.h"
#include "Utils.h"
#include <memory>

// Aliases for readabiliy
constexpr bool Bonus = true;
constexpr bool Penalty = false;

class Histories
{
public:
	
	Histories();

	void ClearAll();
	void ClearKillerAndCounterMoves();

	// Killer move heuristic:
	void SetKillerMove(const Move& move, const int level);
	Move GetKillerMove(const int level) const;
	bool IsKillerMove(const Move& move, const int level) const;
	void ResetKillerForPly(const int level);

	// Countermove heuristic:
	void SetCountermove(const Move& previousMove, const Move& thisMove);
	Move GetCountermove(const Move& previousMove) const;
	bool IsCountermove(const Move& previousMove, const Move& thisMove) const;

	// History heuristic:
	template <bool bonus> void UpdateQuietHistory(const Position& position, const Move& m, const int level, const int depth);
	template <bool bonus> void UpdateCaptureHistory(const Position& position, const Move& m, const int depth);
	int GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const;
	int GetCaptureHistoryScore(const Position& position, const Move& m) const;

	// Correction history for position evaluations:
	void UpdateCorrection(const Position& position, const int16_t refEval, const int16_t score, const int depth);
	int16_t ApplyCorrection(const Position& position, const int16_t rawEval) const;

private:

	inline void UpdateHistoryValue(int16_t& value, const int amount) {
		const int gravity = value * std::abs(amount) / 16384;
		value += amount - gravity;
	}

	// Refutations:
	std::array<Move, MaxDepth> KillerMoves;
	MultiArray<Move, 64, 64> CounterMoves;

	// Move ordering history:
	using QuietHistoryTable = MultiArray<int16_t, 15, 64, 2, 2>;
	using CaptureHistoryTable = MultiArray<int16_t, 15, 64, 15, 2, 2>; // [attacking piece][square to][captured piece]
	using ContinuationHistoryTable = MultiArray<int16_t, 15, 64, 15, 64>;
	QuietHistoryTable QuietHistory;
	CaptureHistoryTable CaptureHistory;
	ContinuationHistoryTable ContinuationHistory;

	// Evaluation correction history:
	using MaterialCorrectionTable = MultiArray<int32_t, 2, 32768>;
	using PawnsCorrectionTable = MultiArray<int32_t, 2, 16384>;
	using FollowUpCorrectionTable = MultiArray<int32_t, 15, 64, 15, 64>;
	MaterialCorrectionTable MaterialCorrectionHistory;
	PawnsCorrectionTable PawnsCorrectionHistory;
	FollowUpCorrectionTable FollowUpCorrectionHistory;
};

