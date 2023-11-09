#pragma once
#include <filesystem>
#include <fstream>
#include "Board.h"
#include "Search.h"
#include "Utils.h"

class Datagen
{
public:
	Datagen();
	void Start();
	void SelfPlay(const std::string filename, const SearchParams params, const SearchParams vParams, const EngineSettings settings,
		const int randomPlyBase, const int startingEvalLimit, const int threadId);
	bool Filter(const Board& board, const Move& move, const int eval) const;
	void ShuffleEntries() const;
	void MergeFiles() const;

	std::string ToMarlinformat(const std::pair<std::string, int>& position, const GameState outcome) const;
	// <fen> | <eval> | <wdl>
	// eval: white pov in cp, wdl 1.0 = white win, 0.0 = black win

	uint64_t PositionsAccepted = 0;
	uint64_t PositionsTotal = 0;
	uint64_t Games = 0;
	Clock::time_point StartTime;
	int ThreadCount = 0;

};

