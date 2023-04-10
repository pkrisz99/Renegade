#pragma once

#include "Board.h"
#include "Results.h"
#include "Heuristics.h"
#include "Evaluation.cpp"
#include "Utils.cpp"
#include <tuple>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <random>
#include <filesystem>

/*
* This is the code responsible for performing move selection.
* SearchRecursive() is the main alpha-beta search, and SearchQuiescence() is called in leaf nodes.
* It also handles opening books and communicating the interim search results.
*/

class Search
{
public:
	Search();
	void Reset();
	void ResetStatistics();

	// Perft methods
	const void Perft(Board& board, const int depth, const PerftType type);
	const uint64_t PerftRecursive(Board& board, const int depth, const int originalDepth, const PerftType type);

	// Move search
	const SearchConstraints CalculateConstraints(const SearchParams params, const bool turn);
	Results SearchMoves(Board& board, const SearchParams params, const EngineSettings settings, const bool display);
	int SearchRecursive(Board& board, int depth, const int level, int alpha, int beta, const bool canNullMove);
	int SearchQuiescence(Board& board, const int level, int alpha, int beta, const bool rootNode);
	int StaticEvaluation(Board& board, const int level, bool checkDraws);

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
	bool FollowingPV;

	// Reused variables
	std::vector<Move> MoveList;
	std::array<std::vector<std::tuple<Move, int>>, 64> MoveOrder;
	std::array<Board, 64> Boards;

	std::array<std::array<int, 32>, 32> LMRTable;

};

