#pragma once
#include "Datagen.h"
#include "Evaluation.h"
#include "Magics.h"
#include "Neurals.h"
#include "Position.h"
#include "Reporting.h"
#include "Search.h"
#include "Settings.h"
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
	void PrintHeader() const;
	void DrawBoard(const Position &pos, const uint64_t customBits = 0) const;
	void HandleBench(const bool lengthy);
	void HandleHelp() const;
	void HandleCompiler() const;

	Search Search;
	std::thread SearchThread;
	bool QuitAfterBench = false;

#if defined(_MSC_VER)
	const bool PrettySupport = true;
#else
	const bool PrettySupport = false;
#endif

};

