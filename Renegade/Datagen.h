#pragma once
#include <filesystem>
#include <fstream>
#include <thread>
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

	std::string ToMarlinformat(const std::pair<std::string, int>& position, const GameState outcome) const;
	// <fen> | <eval> | <wdl>
	// eval: white pov in cp, wdl 1.0 = white win, 0.0 = black win
	
	void LowPlyFilter() const;
	void MergeFiles() const;

	uint64_t PositionsAccepted = 0;
	uint64_t PositionsTotal = 0;
	uint64_t Games = 0;
	Clock::time_point StartTime;
	int ThreadCount = 0;

	// Datagen settings:
	const int startingEvalLimit = 500;
	const int softNodeLimit = 5000;
	const int hardNodeLimit = 100000;
	const int depthLimit = 20;
	const int verificationDepth = 10;
	const int randomPlyBase = 8;

};

