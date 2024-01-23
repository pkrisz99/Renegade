#pragma once

#include "Board.h"
#include "Evaluation.h"
#include "Heuristics.h"
#include "Results.h"
#include "Utils.h"
#include "Neurals.h"
#include <atomic>
#include <tuple>
#include <iomanip>
#include <algorithm>
#include <fstream>
//#include <format>
#include <random>

/*
* This is the code responsible for performing move selection.
* SearchRecursive() is the main alpha-beta search, and SearchQuiescence() is called in leaf nodes.
* It also communicates the interim search results.
*/

class Search
{
public:
	Search();
	void ResetState(const bool clearTT);

	void Perft(Board& board, const int depth, const PerftType type) const;
	Results SearchMoves(Board board, const SearchParams params, const EngineSettings settings, const bool display);
	bool StaticExchangeEval(const Board& board, const Move& move, const int threshold) const;

	std::atomic<bool> Aborting = true;
	bool DatagenMode = false;
	Heuristics Heuristics;

private:
	int SearchRecursive(Board& board, int depth, const int level, int alpha, int beta, const bool canNullMove);
	int SearchQuiescence(Board& board, const int level, int alpha, int beta);
	int Evaluate(const Board& board, const int level);
	uint64_t PerftRecursive(Board& board, const int depth, const int originalDepth, const PerftType type) const;
	SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	bool ShouldAbort();
	int DrawEvaluation();
	void ResetStatistics();

	// NNUE
	void SetupAccumulators(const Board& board);
	template <bool push> void UpdateAccumulators(const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece, const int level);

	// Communication
	void PrintInfo(const Results& e, const EngineSettings& settings) const;
	void PrintPretty(const Results& e, const EngineSettings& settings) const;
	void PrintBestmove(const Move& move) const;

	std::unique_ptr<std::array<AccumulatorRepresentation, MaxDepth + 1>> Accumulators;

	std::chrono::high_resolution_clock::time_point StartSearchTime;
	std::array<std::array<uint64_t, 64>, 64> RootNodeCounts;
	uint16_t Age = 0;

	int Depth;
	SearchStatistics Statistics;
	SearchConstraints Constraints;

	// Reused variables / stack
	std::vector<Move> MoveList;
	std::array<std::vector<std::tuple<Move, int>>, MaxDepth> MoveOrder;
	std::array<Board, MaxDepth> Boards;
	std::array<int, MaxDepth> StaticEvalStack;
	std::array<int, MaxDepth> EvalStack;
	std::array<MoveAndPiece, MaxDepth> MoveStack;

	std::array<Move, MaxDepth> ExcludedMoves;

	std::array<std::array<int, 32>, 32> LMRTable;

};

