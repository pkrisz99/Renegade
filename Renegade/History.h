#pragma once
#include "Move.h"
#include "Position.h"
#include "Utils.h"

class History
{
public:
	
	History();
	~History();

	void ClearHistory();
	void ClearKillerAndCounterMoves();

	// Killer move heuristic:
	void AddKillerMove(const Move& move, const int level);
	bool IsKillerMove(const Move& move, const int level) const;
	bool IsFirstKillerMove(const Move& move, const int level) const;
	bool IsSecondKillerMove(const Move& move, const int level) const;
	void ResetKillersForPly(const int level);

	// Countermove heuristic:
	void AddCountermove(const Move& previousMove, const Move& thisMove);
	bool IsCountermove(const Move& previousMove, const Move& thisMove) const;

	// History heuristic:
	void UpdateHistory(const Move& m, const int16_t delta, const uint8_t piece, const int depth, const Position& position,
		const int level, const bool fromSquareAttacked, const bool toSquareAttacked);
	int GetHistoryScore(const Position& position, const Move& m, const int level, const uint8_t movedPiece, const uint64_t opponentAttacks) const;

private:

	inline void UpdateHistoryValue(int16_t& value, const int amount) {
		const int gravity = value * std::abs(amount) / 16384;
		value += amount - gravity;
	}

	std::array<std::array<Move, 2>, MaxDepth> KillerMoves;
	std::array<std::array<Move, 64>, 64> CounterMoves;

	std::array<std::array<std::array<std::array<int16_t, 64>, 14>, 2>, 2> HistoryTables;

	using Continuations = std::array<std::array<std::array<std::array<int16_t, 64>, 14>, 64>, 14>;
	Continuations* ContinuationHistory;
};

