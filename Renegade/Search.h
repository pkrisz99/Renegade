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
	const void Perft(Board b, const int depth, const PerftType type);
	const int PerftRecursive(Board b, const int depth, const int originalDepth, const PerftType type);

	// Move search
	Evaluation SearchMoves(Board &board, const SearchParams params, const EngineSettings settings);
	int SearchRecursive(Board &board, int depth, int level, int alpha, int beta, bool canNullMove);
	int StaticEvaluation(Board &board, const int level);
	const SearchConstraints CalculateConstraints(const SearchParams params, const bool turn);
	int SearchQuiescence(Board board, int level, int alpha, int beta, bool rootNode);

	// Opening book
	void InitOpeningBook();
	const std::string GetBookMove(const uint64_t hash);
	const BookEntry GetBookEntry(const int item);
	const int GetBookSize();

	// Communication
	const void PrintInfo(const Evaluation e, const EngineSettings settings);
	const void PrintBestmove(Move move);

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

