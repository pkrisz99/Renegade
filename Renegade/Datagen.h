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


// Datagen settings

constexpr SearchParams playingParams     { .nodes = 100000, .depth = 25, .softnodes = 10000 };
constexpr SearchParams verificationParams{ .nodes = 100000, .depth = 25, .softnodes = 20000 };

constexpr int randomPlyBaseNormal = 2;
constexpr int randomPlyBaseDFRC = 4;
constexpr int startingEvalLimit = 500;

constexpr int drawAdjEvalThreshold = 5;
constexpr int drawAdjPlies = 15;
constexpr int winAdjEvalThreshold = 2000;
constexpr int winAdjEvalPlies = 5;

enum class DatagenLaunchMode { Ask, Normal, DFRC };
void StartDatagen(const DatagenLaunchMode launchMode);

// Viriformat is a modern and efficient way of storing games and evals from datagen
// Credit goes to Cosmo (author of Viridithas) for coming up with this

class ViriformatGame {
public:
	ViriformatGame() {
		moves.reserve(300);
	}
	void SetStartingBoard(const Board& b, const CastlingConfiguration& cc);
	void AddMove(const Move& move, const int eval);
	void Finish(const GameState state);
	uint16_t ToViriformatMove(const Move& m) const;
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
