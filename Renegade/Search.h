#pragma once

#include "Board.h"
#include "Results.h"
#include "Heuristics.h"
#include "Evaluation.cpp"
#include "Utils.cpp"
#include <tuple>
#include <iomanip>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <random>
#include <filesystem>

class Search
{
public:
	Search();
	void Reset();
	void ResetStatistics();

	// Perft methods
	const void Perft(Board b, const int depth, const PerftType type);
	const int PerftRecursive(Board b, const int depth, const int originalDepth, const PerftType type);

	// Move search
	Results SearchMoves(Board &board, const SearchParams params, const EngineSettings settings);
	int SearchRecursive(Board &board, int depth, int level, int alpha, int beta, bool canNullMove);
	int StaticEvaluation(Board &board, const int level, bool checkDraws);
	const SearchConstraints CalculateConstraints(const SearchParams params, const bool turn);
	int SearchQuiescence(Board &board, int level, int alpha, int beta, bool rootNode);
	const int CalculateDeltaMargin(Board& board);

	// Opening book
	void InitOpeningBook();
	const std::string GetBookMove(const uint64_t hash);
	const BookEntry GetBookEntry(const int item);
	const int GetBookSize();

	// Communication
	const void PrintInfo(const Results e, const EngineSettings settings);
	const void PrintBestmove(Move move);

	int Depth;
	SearchStatistics Statistics;

	std::vector<BookEntry> BookEntries;
	Heuristics Heuristics;
	SearchConstraints Constraints;
	bool Aborting = false;
	std::chrono::steady_clock::time_point StartSearchTime;

	// Reused variables
	std::vector<Move> MoveList;
	std::vector<std::tuple<Move, int>> MoveOrder[100];
	std::array<Board, 100> Boards;

};

