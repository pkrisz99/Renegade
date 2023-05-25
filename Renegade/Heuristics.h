#pragma once
#include "Results.h"
#include "Board.h"
#include "Evaluation.cpp"
#include "Utils.cpp"
#include <algorithm>
#include <array>
#include <memory>

/*
* Collection of functions to make search more efficient.
* Handles the transposition table, history heuristic, killer moves, and the PV table.
*/

namespace ScoreType {
	static const int Exact = 0;
	static const int UpperBound = 1; // Alpha (fail-low)
	static const int LowerBound = 2; // Beta (fail-high)
};

struct TranspositionEntry {
	uint32_t hash;
	int32_t score;
	uint8_t depth;
	uint8_t scoreType;
	uint8_t moveFrom, moveTo, moveFlag;
};

class Heuristics
{
public:
	static const int PvSize = 32;

	Heuristics();
	const int CalculateOrderScore(Board& board, const Move& m, const int level, const float phase, const bool onPv, const Move& ttMove, 
		const Move& previousMove, const bool losingCapture);
	
	// PV table
	void UpdatePvTable(const Move& move, const int level, const bool leaf);
	void SetPvLine(const std::vector<Move> pv);
	const std::vector<Move> GeneratePvLine();
	const bool IsPvMove(const Move& move, const int level);
	void ClearPvLine();
	void ResetPvTable();

	// Killer and countermoves
	void AddKillerMove(const Move& m, const int level);
	const bool IsCountermove(const Move& previousMove, const Move& thisMove);
	const bool IsKillerMove(const Move& move, const int level);
	const bool IsFirstKillerMove(const Move& move, const int level);
	const bool IsSecondKillerMove(const Move& move, const int level);
	void ClearKillerAndCounterMoves();
	const void AddCountermove(const Move& previousMove, const Move& thisMove);

	// History heuristic
	void IncrementHistory(const bool side, const int from, const int to, const int depth);
	void DecrementHistory(const bool side, const int from, const int to, const int depth);
	void AgeHistory();
	void ClearHistoryTable();

	// Transposition table
	void AddTranspositionEntry(const uint64_t hash, const int depth, int score, const int scoreType, const Move bestMove, const int level);
	const bool RetrieveTranspositionEntry(const uint64_t& hash, const int depth, TranspositionEntry& entry, const int level);
	void SetHashSize(const int megabytes);
	const int GetHashfull();
	const void GetTranspositionInfo(uint64_t& ttTheoretical, uint64_t& ttUsable, uint64_t& ttBits, uint64_t& ttUsed);
	void ClearTranspositionTable();

	Move PvTable[PvSize + 1][PvSize + 1];


private:
	std::vector<TranspositionEntry> TranspositionTable;
	uint64_t HashFilter;
	int HashBits;
	int TranspositionEntryCount;
	int TheoreticalTranspositionEntires;

	std::array<std::array<Move, 2>, 32> KillerMoves;
	std::vector<Move> PvMoves;
	std::array<std::array<std::array<int, 64>, 64>, 2> HistoryTables;
	std::array<std::array<Move, 64>, 64> CounterMoves;

};

