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
	std::array<std::array<Move, 64>, 64> CounterMoves;

	// Move ordering history:
	using ThreatBuckets = std::array<std::array<int16_t, 2>, 2>;
	using QuietHistoryTable = std::array<std::array<ThreatBuckets, 64>, 14>;
	using CaptureHistoryTable = std::array<std::array<std::array<int16_t, 14>, 64>, 14>;  // [attacking piece][square to][captured piece]
	using ContinuationHistoryTable = std::array<std::array<std::array<std::array<int16_t, 64>, 14>, 64>, 14>;

	QuietHistoryTable QuietHistory;
	CaptureHistoryTable CaptureHistory;
	ContinuationHistoryTable ContinuationHistory;

	// Evaluation correction history:
	using MaterialCorrectionTable = std::array<std::array<int32_t, 32768>, 2>;
	using PawnsCorrectionTable = std::array<std::array<int32_t, 16384>, 2>;
	MaterialCorrectionTable MaterialCorrectionHistory;
	PawnsCorrectionTable PawnsCorrectionHistory;
};

