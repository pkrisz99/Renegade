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
	int GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const;

    // Static evaluation correction history:
    void UpdateCorrection(const Position& position, const int staticEval, const int score, const int depth);
    int AdjustStaticEvaluation(const Position& position, const int staticEval) const;

private:

	inline void UpdateHistoryValue(int16_t& value, const int amount) {
		const int gravity = value * std::abs(amount) / 16384;
		value += amount - gravity;
	}

	// Refutations:
	std::array<Move, MaxDepth> KillerMoves;
	std::array<std::array<Move, 64>, 64> CounterMoves;

	// History:
	using ThreatBuckets = std::array<std::array<int16_t, 2>, 2>;
	using ThreatHistoryTable = std::array<std::array<ThreatBuckets, 64>, 14>;
	using ContinuationTable = std::array<std::array<std::array<std::array<int16_t, 64>, 14>, 64>, 14>;

	ThreatHistoryTable QuietHistory;
	ContinuationTable Continuations;
    std::array<std::array<int, 32768>, 2> MaterialCorrectionHistory;
};

