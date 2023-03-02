#pragma once
#include "Results.h"
#include "Board.h"
#include "Evaluation.cpp"
#include "Utils.cpp"
#include <algorithm>
#include <array>
#include <unordered_map>

namespace ScoreType {
	static const int Exact = 0;
	static const int UpperBound = 1; // Alpha (fail-low)
	static const int LowerBound = 2; // Beta (fail-high)
};

struct HashEntry {
	int score;
	int scoreType;
};

class Heuristics
{
public:
	static const int PvSize = 20;

	Heuristics();
	void AddEntry(const uint64_t hash, const int score, const int scoreType);
	void AddKillerMove(const Move m, const int level);
	const bool IsKillerMove(const Move move, const int level);
	const bool IsPvMove(const Move move, const int level);
	void SetPv(const std::vector<Move> pv);
	void ClearEntries();
	void ClearPv();
	const std::tuple<bool, HashEntry> RetrieveEntry(const uint64_t &hash);
	void SetHashSize(const int megabytes);
	const int GetHashfull();
	void UpdatePvTable(const Move move, const int level, const bool leaf);
	const std::vector<Move> GetPvLine();
	const int CalculateOrderScore(Board board, const Move m, const int level, const float phase, const bool onPv);
	const int64_t EstimateAllocatedMemory();
	void ResetHashStructure();
	void AddCutoffHistory(const bool side, const int from, const int to, const int depth);

	std::unordered_map<uint64_t, HashEntry> Hashes;
	int HashedEntryCount;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	int64_t MaximumHashMemory;
	Move PvTable[PvSize + 1][PvSize + 1];
	std::array < std::array<std::array<int, 64>, 64>, 2> HistoryTables;

	const HashEntry NoEntry = { -1, ScoreType::Exact};

};

