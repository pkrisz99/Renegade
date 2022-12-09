#pragma once
#include <unordered_map>
#include "Evaluation.h"
#include "Utils.cpp"
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
	std::vector<Move> moves;
};

class Heuristics
{
public:
	Heuristics();
	void AddEntry(unsigned __int64 hash, eval e, int scoreType);
	void AddKillerMove(Move m, int level);
	bool IsKillerMove(Move move, int level);
	bool IsPvMove(Move move, int level);
	void SetPv(std::vector<Move> pv);
	void ClearEntries();
	void ClearPv();
	std::tuple<bool, HashEntry> RetrieveEntry(unsigned __int64 hash);
	void SetHashSize(int megabytes);
	int GetHashfull();

	std::unordered_map<unsigned __int64, HashEntry> Hashes;
	int HashedEntryCount;
	int ApproxHashSize;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;
	unsigned __int64 MaximumHashSize;

	static const unsigned __int64 PlyHash = 0x2022b;
	const HashEntry NoEntry = { -1, ScoreType::Exact, std::vector<Move>() };
};

