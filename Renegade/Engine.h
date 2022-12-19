#pragma once
#include "Board.h"
#include "Evaluation.h"
#include "Heuristics.h"
#include <string>
#include <tuple>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <random>
#include <filesystem>
#include "Search.h"

class Engine
{
public:
	Engine();
	void Start();
	void Play();
	Search Search;
	EngineSettings Settings;
};

