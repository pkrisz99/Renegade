#pragma once
#include "Board.h"
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


class Engine
{
public:
	Engine();
	void Start();
	void Play();
	Search Search;
	EngineSettings Settings;
};

