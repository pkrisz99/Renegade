#pragma once
#include "Search.h"
#include "Utils.h"
#include <thread>

class Engine
{
public:
	void StartSearch(Position pos, SearchParams sp);
	void StopSearch();
	bool IsBusy() const;
	void JoinThreads();

	void ClearHash();
	void SetHashSize(const int megabytes);
	void Reset();

private:
	Search Searcher;
	std::thread SearchThread;

	Position position;
	SearchParams params;

	//Transpositions TranspositionTable;
};

