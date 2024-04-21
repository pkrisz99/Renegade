#pragma once

#include "Heuristics.h"
#include "Neurals.h"
#include "Position.h"
#include "Reporting.h"
#include "Utils.h"
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
*/

class Search
{
public:
	Search();
	void ResetState(const bool clearTT);

	void Perft(Position& position, const int depth, const PerftType type) const;
	Results SearchMoves(Position& position, const SearchParams params, const bool display);
	bool StaticExchangeEval(const Position& position, const Move& move, const int threshold) const;

	std::atomic<bool> Aborting = true;
	bool DatagenMode = false;
	Heuristics Heuristics;

private:
	int SearchRecursive(Position& position, int depth, const int level, int alpha, int beta, const bool canNullMove);
	int SearchQuiescence(Position& position, const int level, int alpha, int beta);
	int Evaluate(const Position& position, const int level);
	uint64_t PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const;
	SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	bool ShouldAbort();
	int DrawEvaluation() const;
	void ResetStatistics();

	void OrderMoves(const Position& position, MoveList& ml, const int level, const Move& ttMove, const uint64_t opponentAttacks);
	void OrderMovesQ(const Position& position, MoveList& ml, const int level);

	// NNUE
	void SetupAccumulators(const Position& position);
	void UpdateAccumulators(const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece, const int level);

	std::unique_ptr<std::array<AccumulatorRepresentation, MaxDepth + 1>> Accumulators;

	std::chrono::high_resolution_clock::time_point StartSearchTime;
	std::array<std::array<uint64_t, 64>, 64> RootNodeCounts;
	uint16_t Age = 0;

	int Depth;
	SearchStatistics Statistics;
	SearchConstraints Constraints;

	// Reused variables / stack
	std::array<MoveList, MaxDepth> MoveListStack{};
	std::array<int, MaxDepth> StaticEvalStack;
	std::array<int, MaxDepth> EvalStack;
	std::array<int, MaxDepth> CutoffCount;
	std::array<int, MaxDepth> DoubleExtensions;
	std::array<Move, MaxDepth> ExcludedMoves;

	std::array<std::array<int, 32>, 32> LMRTable;

};

