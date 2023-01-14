#pragma once
#include "Evaluation.h"
#include "Board.h"
#include "Utils.cpp"
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
	const std::tuple<bool, HashEntry> RetrieveEntry(const uint64_t hash);
	void SetHashSize(const int megabytes);
	const int GetHashfull();
	void UpdatePvTable(const Move move, const int level, const bool leaf);
	const std::vector<Move> GetPvLine();
	const int CalculateOrderScore(Board board, const Move m, const int level, const float phase);

	std::unordered_map<uint64_t, HashEntry> Hashes;
	int HashedEntryCount;
	int64_t ApproxHashSize;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	uint64_t MaximumHashSize;
	Move PvTable[PvSize][PvSize];

	const HashEntry NoEntry = { -1, ScoreType::Exact};

};

