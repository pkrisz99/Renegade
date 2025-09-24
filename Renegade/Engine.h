#pragma once
#include "Datagen.h"
#include "Neural.h"
#include "Position.h"
#include "Reporting.h"
#include "Search.h"
#include "Settings.h"
#include <fstream>
#include <iomanip>
#include <random>
#include <thread>
#include <tuple>

void GenerateMagicTables();

enum class EngineBehavior { Normal, Bench, DatagenNormal, DatagenDFRC };

class Engine
{
public:
	Engine(int argc, char* argv[]);
	void Start();
	void PrintHeader() const;
	void DrawBoard(const Position &pos, const uint64_t highlight = 0) const;
	void HandleBench();
	void HandleHelp() const;
	void HandleCompiler() const;

	Search SearchThreads;
	EngineBehavior Behavior = EngineBehavior::Normal;

#ifdef __cpp_lib_format
	const bool PrettySupport = true;
#else
	const bool PrettySupport = false;
#endif

};

