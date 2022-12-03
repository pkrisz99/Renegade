#pragma once
#include <unordered_map>
#include "Evaluation.h"
#include "Utils.cpp"
#include <queue>
#include <array>

struct HashEntry {
	int score;
	std::vector<Move> moves;
};

class Heuristics
{
public:
	Heuristics();
	void AddEntry(unsigned __int64 hash, eval e, int level);
	void AddKillerMove(Move m, int level);
	bool IsKillerMove(Move move, int level);
	bool IsPvMove(Move move, int level);
	void SetPv(std::vector<Move> pv);
	void ClearEntries();
	void ClearPv();
	std::tuple<bool, HashEntry> RetrieveEntry(unsigned __int64 hash, int level);

	std::unordered_map<unsigned __int64, HashEntry> Hashes;
	int HashedEntryCount;
	std::vector<std::array<Move, 2>> KillerMoves;
	std::vector<Move> PvMoves;

	static const unsigned __int64 PlyHash = 0x2022b;
	const HashEntry NoEntry = { -1, std::vector<Move>() };
};

