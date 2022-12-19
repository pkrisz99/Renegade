#pragma once

#include "Board.h"
#include "Evaluation.h"
#include "Heuristics.h"
#include "Utils.cpp"
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

class Search
{
public:
	Search();
	void Reset();

	// Perft methods
	void Perft(Board b, int depth, PerftType type);
	int PerftRecursive(Board b, int depth, int originalDepth, PerftType type);

	// Move search
	Evaluation SearchMoves(Board board, SearchParams params, EngineSettings settings);
	int SearchRecursive(Board board, int depth, int level, int alpha, int beta, bool canNullMove);
	int StaticEvaluation(Board board, int level);
	SearchConstraints CalculateConstraints(SearchParams params, bool turn);

	// Opening book
	void InitOpeningBook();
	std::string GetBookMove(uint64_t hash);
	BookEntry GetBookEntry(int item);
	int GetBookSize();

	// Communication
	void PrintInfo(Evaluation e);
	void PrintBestmove(Move move);

	int EvaluatedNodes;
	int EvaluatedQuiescenceNodes;
	int SelDepth;
	int Depth;

	std::vector<BookEntry> BookEntries;
	Heuristics Heuristics;
	SearchConstraints Constraints;
	bool Aborting = false;
	std::chrono::steady_clock::time_point StartSearchTime;
	bool Explored;

};

