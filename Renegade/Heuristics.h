#pragma once
#include <unordered_map>;
#include "Evaluation.h"
#include "Utils.cpp"

struct HashEntry {
	int score;
	std::vector<Move> moves;
};

class Heuristics
{
public:
	Heuristics();
	void AddEntry(unsigned __int64 hash, eval e, int level);
	void ClearEntries();
	std::tuple<bool, HashEntry> RetrieveEntry(unsigned __int64 hash, int level);

	std::unordered_map<unsigned __int64, HashEntry> Hashes;
	int HashedEntryCount;

	static const unsigned __int64 PlyHash = 0x2022b;
	const HashEntry NoEntry = { -1, std::vector<Move>() };
};

