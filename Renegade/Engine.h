#pragma once
#include "Board.h"
#include "Magics.h"
#include "Heuristics.h"
#include "Results.h"
#include "Search.h"
#include "Tuning.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <thread>
#include <tuple>

extern void GenerateMagicTables();

class Engine
{
public:
	Engine(int argc, char* argv[]);
	void Start();
	void Play();
	void DrawBoard(Board b, uint64_t customBits = 0);
	void HandleBench();
	void PrintHeader();

	Search Search;
	EngineSettings Settings;
};

