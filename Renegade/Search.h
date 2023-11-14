#pragma once

#include "Board.h"
#include "Evaluation.h"
#include "Heuristics.h"
#include "Results.h"
#include "Utils.h"
#include "Neurals.h"
#include <tuple>
#include <iomanip>
#include <algorithm>
#include <fstream>
//#include <format>
#include <random>

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
	void Perft(Board& board, const int depth, const PerftType type) const;
	uint64_t PerftRecursive(Board& board, const int depth, const int originalDepth, const PerftType type) const;

	// Move search
	const SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	inline bool ShouldAbort() const;
	const Results SearchMoves(Board board, const SearchParams params, const EngineSettings settings, const bool display);
	int SearchRecursive(Board& board, int depth, const int level, int alpha, int beta, const bool canNullMove);
	int SearchQuiescence(Board& board, const int level, int alpha, int beta);
	int Evaluate(const Board& board);
	bool StaticExchangeEval(const Board& board, const Move& move, const int threshold);
	int DrawEvaluation();

	// Communication
	void PrintInfo(const Results& e, const EngineSettings& settings);
	void PrintPretty(const Results& e, const EngineSettings& settings);
	void PrintBestmove(const Move& move);

	void ResetState(const bool clearTT);

	int Depth;
	SearchStatistics Statistics;

	Heuristics Heuristics;
	SearchConstraints Constraints;
	bool Aborting = false;
	std::chrono::high_resolution_clock::time_point StartSearchTime;
	uint16_t Age = 0;
	bool DatagenMode = false;

	// Reused variables / stack
	std::vector<Move> MoveList;
	std::array<std::vector<std::tuple<Move, int>>, MaxDepth> MoveOrder;
	std::array<Board, MaxDepth> Boards;
	std::array<Move, MaxDepth> MoveStack;
	std::array<std::array<bool, 256>, MaxDepth> LegalAndQuiet;
	std::array<int, MaxDepth> EvalStack;

	std::array<std::array<int, 32>, 32> LMRTable;

};

