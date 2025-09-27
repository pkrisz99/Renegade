#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <thread>

#ifdef __cpp_lib_format
#include <format>
#endif

#include "Position.h"
#include "Search.h"
#include "Settings.h"
#include "Utils.h"


// Datagen settings:
constexpr int startingEvalLimit = 500;
constexpr int verificationDepth = 10;
constexpr int softNodeLimit = 5000;
constexpr int hardNodeLimit = 500000;
constexpr int depthLimit = 20;
constexpr int randomPlyBaseNormal = 2;
constexpr int randomPlyBaseDFRC = 4;
constexpr int minSavePly = 16;

constexpr int drawAdjEvalThreshold = 5;
constexpr int drawAdjPlies = 15;
constexpr int winAdjEvalThreshold = 2000;
constexpr int winAdjEvalPlies = 5;

enum class DatagenLaunchMode { Ask, Normal, DFRC };

void MergeDatagenFiles();
void StartDatagen(const DatagenLaunchMode launchMode);

// Viriformat is a modern and efficient way of storing games and evals from datagen
// Credit goes to Cosmo (author of Viridithas) for coming up with this

class Viriformat {
public:
	Viriformat() {
		moves.reserve(300);
	}
	void SetStartingBoard(const Board& b, const CastlingConfiguration& cc);
	void AddMove(const Move& move, const int eval);
	void Finish(const GameState state);
	uint16_t ViriformatMove(const Move& m) const;
	void WriteToFile(std::ofstream& stream) const;

private:
	Board startingBoard;
	CastlingConfiguration castlingConfig;
	std::vector<std::pair<uint16_t, int16_t>> moves; // [move, eval]
	GameState outcome = GameState::Playing;

	// Renegade makes use of the extra byte in the spec to store version information
	// -> highest bit: whether the datagen is DFRC
	// -> low 7 bits: version (e.g. v1.2.5 -> 1)
	static constexpr uint8_t datagenVersion = 1;
};
