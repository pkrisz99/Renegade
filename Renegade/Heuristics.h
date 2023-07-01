#pragma once
#include "Board.h"
#include "Evaluation.h"
#include "Results.h"
#include "Utils.h"
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
	static const int PvSize = 64;

	Heuristics();
	int CalculateOrderScore(Board& board, const Move& m, const int level, const float phase, const bool onPv, const Move& ttMove, 
		const Move& previousMove, const bool losingCapture);
	
	// PV table
	void UpdatePvTable(const Move& move, const int level);
	void InitPvLength(const int level);
	void SetPvLine(const std::vector<Move> pv);
	void GeneratePvLine(std::vector<Move>& list) const;
	bool IsPvMove(const Move& move, const int level) const;
	void ClearPvLine();
	void ResetPvTable();

	// Killer and countermoves
	void AddKillerMove(const Move& m, const int level);
	bool IsCountermove(const Move& previousMove, const Move& thisMove);
	bool IsKillerMove(const Move& move, const int level);
	bool IsFirstKillerMove(const Move& move, const int level);
	bool IsSecondKillerMove(const Move& move, const int level);
	void ClearKillerAndCounterMoves();
	void AddCountermove(const Move& previousMove, const Move& thisMove);

	// History heuristic
	void IncrementHistory(const bool side, const int from, const int to, const int depth);
	void DecrementHistory(const bool side, const int from, const int to, const int depth);
	void AgeHistory();
	void ClearHistoryTable();

	// Transposition table
	void AddTranspositionEntry(const uint64_t hash, const int depth, int score, const int scoreType, const Move bestMove, const int level);
	bool RetrieveTranspositionEntry(const uint64_t& hash, TranspositionEntry& entry, const int level);
	void SetHashSize(const int megabytes);
	int GetHashfull();
	void GetTranspositionInfo(uint64_t& ttTheoretical, uint64_t& ttUsable, uint64_t& ttBits, uint64_t& ttUsed);
	void ClearTranspositionTable();

	std::array<std::array<Move, PvSize + 1>, PvSize + 1> PvTable;
	std::array<int, PvSize + 1> PvLength;


private:
	std::vector<TranspositionEntry> TranspositionTable;
	uint64_t HashFilter;
	int HashBits;
	uint64_t TranspositionEntryCount;
	uint64_t TheoreticalTranspositionEntires;

	std::array<std::array<Move, 2>, 32> KillerMoves;
	std::vector<Move> PvMoves;
	std::array<std::array<std::array<int, 64>, 64>, 2> HistoryTables;
	std::array<std::array<Move, 64>, 64> CounterMoves;

};

