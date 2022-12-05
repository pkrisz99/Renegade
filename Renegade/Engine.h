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

typedef std::chrono::high_resolution_clock Clock;

class Engine
{
public:
	Engine();
	void perft(std::string fen, int depth, bool verbose);
	void perft(Board b, int depth, bool verbose);
	int perft1(Board board, int depth, bool verbose);
	int perftRecursive(Board b, int depth);
	Evaluation Search(Board board);
	eval SearchRecursive(Board board, int depth, int level, int alpha, int beta);
	int StaticEvaluation(Board board, int level);
	void Start();
	void PrintInfo(Evaluation e);
	void Play();
	void InitOpeningBook();
	std::string GetBookMove(unsigned __int64 hash);
	SearchConstraints CalculateConstraints(SearchParams params, bool turn);

	int EvaluatedNodes;
	int EvaluatedQuiescenceNodes;
	std::vector<BookEntry> BookEntries;
	int HashSize;
	int SelDepth;
	int Depth;
	EngineSettings Settings;
	Heuristics Heuristics;
	SearchConstraints Constraints;
	bool Aborting = false;
	std::chrono::steady_clock::time_point StartSearchTime;
};

