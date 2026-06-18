#pragma once
#include "Datagen.h"
#include "Classical.h"
#include "Position.h"
#include "Reporting.h"
#include "Search.h"
#include "Settings.h"
#include <fstream>
#include <iomanip>
#include <thread>
#include <tuple>

void GenerateMagicTables();

enum class EngineBehavior { Normal, Bench, Datagen };

class Engine {
public:
	Engine(int argc, char* argv[]);
	void Start();

private:
	void PrintHeader() const;
	void HandleDraw(const Position& pos, const uint64_t highlight = 0) const;
	void HandleBench();
	void HandleSetOption(const std::vector<std::string>& parts);
	void HandlePosition(const std::string originalInput);
	void HandleGo(const std::vector<std::string>& parts);
	void HandleHelp() const;
	void HandleNNUE() const;
	void HandleCompiler() const;
	void Perft(Position& position, const int depth, const PerftType type) const;
	uint64_t PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const;

	Search searchThreads;
	EngineBehavior behavior = EngineBehavior::Normal;
	Position position = Position();

#ifdef RENEGADE_DATAGEN
	DatagenLaunchSettings datagenSettings{};
#endif

};
