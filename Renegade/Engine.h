#pragma once
#include "Datagen.h"
#include "Magics.h"
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

enum class EngineBehavior { Normal, Bench, Datagen };

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

#if defined(_MSC_VER)
	const bool PrettySupport = true;
#else
	const bool PrettySupport = false;
#endif

};

