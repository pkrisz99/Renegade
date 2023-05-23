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
	Engine();
	void Start();
	void Play();
	const void DrawBoard(Board b, uint64_t customBits = 0);
	const void PrintHeader();

	Search Search;
	EngineSettings Settings;
};

