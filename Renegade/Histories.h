#pragma once
#include "Move.h"
#include "Position.h"
#include "Utils.h"
#include <memory>

class Histories
{
public:
	
	Histories();

	void ClearAll();
	void ClearKillerAndCounterMoves();

	// Killer move heuristic:
	void SetKillerMove(const Move& move, const int level);
	bool IsKillerMove(const Move& move, const int level) const;
	void ResetKillerForPly(const int level);

	// Countermove heuristic:
	void SetCountermove(const Move& previousMove, const Move& thisMove);
	bool IsCountermove(const Move& previousMove, const Move& thisMove) const;

	// History heuristic (quiet moves):
	void UpdateHistory(const Position& position, const Move& m, const uint8_t piece, const int16_t delta, const int level);
	void UpdateCaptureHistory(const Position& position, const Move& m, const int16_t delta);
	int GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const;
	int16_t GetCaptureHistoryScore(const Position& position, const Move& m) const;

	// Static evaluation correction history:
	void UpdateCorrection(const Position& position, const int16_t rawEval, const int16_t score, const int depth);
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

	using QuietHistoryStructureTable = MultiArray<uint16_t, 15, 64, 2, 2>;

	QuietHistoryTable QuietHistory;
	CaptureHistoryTable CaptureHistory;
	ContinuationHistoryTable ContinuationHistory;
	QuietHistoryStructureTable QuietHistoryStructure;

	// Evaluation correction history:
	using MaterialCorrectionTable = MultiArray<int32_t, 2, 32768>;
	using PawnsCorrectionTable = MultiArray<int32_t, 2, 16384>;
	using FollowUpCorrectionTable = MultiArray<int32_t, 15, 64, 15, 64>;
	MaterialCorrectionTable MaterialCorrectionHistory;
	PawnsCorrectionTable PawnsCorrectionHistory;
	FollowUpCorrectionTable FollowUpCorrectionHistory;
};

