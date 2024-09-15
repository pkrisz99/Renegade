#pragma once
#include <filesystem>
#include <fstream>
#include <optional>
#include <thread>
#include "Position.h"
#include "Search.h"
#include "Settings.h"
#include "Utils.h"

struct DatagenSettings {
	std::string filename{};
	unsigned int threadCount = 0;
	bool doFRC = false;
};

class Datagen
{
public:
	void Start(const std::optional<DatagenSettings> datagenSettings);
	void MergeFiles() const;

private:

	// Datagen settings:
	const int startingEvalLimit = 500;
	const int verificationDepth = 10;
	const int softNodeLimit = 5000;
	const int hardNodeLimit = 500000;
	const int depthLimit = 20;
	const int randomPlyBase = 10;
	const int minSavePly = 16;

	void SelfPlay(const std::string filename);
	bool Filter(const Position& pos, const Move& move, const int eval) const;
	std::string ToTextformat(const std::string fen, const int16_t whiteScore, const GameState outcome) const;

	std::atomic<uint64_t> PositionsAccepted = 0;
	std::atomic<uint64_t> PositionsTotal = 0;
	std::atomic<uint64_t> Games = 0;

	Clock::time_point StartTime;
	int ThreadCount = 0;
	bool DFRC = false;

};
