#pragma once
#include "Results.h"
#include "Board.h"
#include "Evaluation.cpp"
#include "Utils.cpp"
#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>

namespace ScoreType {
	static const int Exact = 0;
	static const int UpperBound = 1; // Alpha (fail-low)
	static const int LowerBound = 2; // Beta (fail-high)
};

struct TranspositionEntry {
	int32_t score;
	uint8_t depth;
	uint8_t scoreType;
	uint64_t hash;
	uint8_t moveFrom, moveTo, moveFlag;
};

class Heuristics
{
public:
	static const int PvSize = 20;

	Heuristics();
	void ClearEntries();
	const int CalculateOrderScore(Board& board, const Move& m, const int level, const float phase, const bool onPv, const Move& trMove);
	
	// PV table
	void UpdatePvTable(const Move& move, const int level, const bool leaf);
	void SetPvLine(const std::vector<Move> pv);
	const std::vector<Move> GeneratePvLine();
	const bool IsPvMove(const Move& move, const int level);
	void ClearPvLine();
	void ResetPvTable();

	// Killer moves
	void AddKillerMove(const Move& m, const int level);
	const bool IsKillerMove(const Move& move, const int level);
	void ClearKillerMoves();

	// History heuristic
	void AddCutoffHistory(const bool side, const int from, const int to, const int depth);
	void ClearHistoryTable();

	// Transposition table
	void AddTranspositionEntry(const uint64_t hash, const int depth, const int score, const int scoreType, const Move bestMove);
	const bool RetrieveTranspositionEntry(const uint64_t& hash, const int depth, TranspositionEntry& entry);
	void SetHashSize(const int megabytes);
	const int GetHashfull();
	const void GetTranspositionInfo(uint64_t& trTheoretical, uint64_t& trUsable, uint64_t& trBits, uint64_t& trUsed);
	void ClearTranspositionTable();


private:
	std::vector<TranspositionEntry> TranspositionTable;
	uint64_t HashFilter;
	int HashBits;
	int TranspositionEntryCount;
	int TheoreticalTranspositionEntires;

	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	Move PvTable[PvSize + 1][PvSize + 1];
	std::array<std::array<std::array<int, 64>, 64>, 2> HistoryTables;

};

