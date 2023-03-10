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
	int score;
	uint8_t depth;
	uint8_t scoreType;
};

class Heuristics
{
public:
	static const int PvSize = 20;

	Heuristics();
	void ClearEntries();
	const int CalculateOrderScore(Board& board, const Move& m, const int level, const float phase, const bool onPv);
	
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
	void AddTranspositionEntry(const uint64_t hash, const int depth, const int score, const int scoreType);
	const bool RetrieveTranspositionEntry(const uint64_t& hash, TranspositionEntry& entry);
	const int64_t EstimateTranspositionAllocations();
	void SetHashSize(const int megabytes);
	const int GetHashfull();
	void ClearTranspositionTable();
	void ResetTranspositionAllocations();

	// Variables
	std::unordered_map<uint64_t, TranspositionEntry> Hashes;
	int HashedEntryCount;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	int64_t MaximumHashMemory;
	Move PvTable[PvSize + 1][PvSize + 1];
	std::array<std::array<std::array<int, 64>, 64>, 2> HistoryTables;

};

