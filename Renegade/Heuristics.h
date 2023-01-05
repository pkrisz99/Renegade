#pragma once
#include <unordered_map>
#include "Evaluation.h"
#include "Utils.cpp"
#include "Board.h"
#include <queue>
#include <array>

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
	Heuristics();
	void AddEntry(const uint64_t hash, const int score, const int scoreType);
	void AddKillerMove(const Move m, const int level);
	const bool IsKillerMove(const Move move, const int level);
	const bool IsPvMove(const Move move, const int level);
	void SetPv(const std::vector<Move> pv);
	void ClearEntries();
	void ClearPv();
	const std::tuple<bool, HashEntry> RetrieveEntry(const uint64_t hash);
	void SetHashSize(const int megabytes);
	const int GetHashfull();
	void UpdatePvTable(const Move move, const int level);
	const std::vector<Move> GetPvLine();
	const int CalculateMoveOrderScore(Board board, const Move m, const int level, const float phase);

	std::unordered_map<uint64_t, HashEntry> Hashes;
	int HashedEntryCount;
	int64_t ApproxHashSize;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	uint64_t MaximumHashSize;
	Move PvTable[30][30];

	const HashEntry NoEntry = { -1, ScoreType::Exact};
};

