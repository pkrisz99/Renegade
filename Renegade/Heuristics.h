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
	void AddEntry(uint64_t hash, int score, int scoreType);
	void AddKillerMove(Move m, int level);
	bool IsKillerMove(Move move, int level);
	bool IsPvMove(Move move, int level);
	void SetPv(std::vector<Move> pv);
	void ClearEntries();
	void ClearPv();
	std::tuple<bool, HashEntry> RetrieveEntry(uint64_t hash);
	void SetHashSize(int megabytes);
	int GetHashfull();
	void UpdatePvTable(Move move, int level);
	std::vector<Move> GetPvLine();
	int CalculateMoveOrderScore(Board board, Move m, int level);

	std::unordered_map<uint64_t, HashEntry> Hashes;
	int HashedEntryCount;
	int64_t ApproxHashSize;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	uint64_t MaximumHashSize;
	Move PvTable[30][30];

	const HashEntry NoEntry = { -1, ScoreType::Exact};
};

