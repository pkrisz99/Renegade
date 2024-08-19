#pragma once
#include "Histories.h"
#include "Movepicker.h"
#include "Neural.h"
#include "Position.h"
#include "Reporting.h"
#include "Transpositions.h"
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
	Results StartSearch(Position& position, const SearchParams params, const bool display);
	bool StaticExchangeEval(const Position& position, const Move& move, const int threshold) const;

	std::atomic<bool> Aborting = true;
	bool DatagenMode = false;
	Transpositions TranspositionTable;

private:
	Results SearchMoves(Position& position, const bool display);
	int SearchRecursive(Position& position, int depth, const int level, int alpha, int beta);
	int SearchQuiescence(Position& position, const int level, int alpha, int beta);

	int Evaluate(const Position& position, const int level);
	uint64_t PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const;
	SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	bool ShouldAbort();
	int DrawEvaluation() const;
	void ResetStatistics();

	void OrderMoves(const Position& position, MoveList& ml, const int level, const Move& ttMove);
	void OrderMovesQ(const Position& position, MoveList& ml, const int level);
	int CalculateOrderScore(const Position& position, const Move& m, const int level, const Move& ttMove,
		const bool losingCapture, const bool useMoveStack) const;

	// NNUE
	void SetupAccumulators(const Position& position);
	void UpdateAccumulators(const Position& pos, const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece, const int level);

	// PV table
	void UpdatePvTable(const Move& move, const int level);
	void InitPvLength(const int level);
	std::vector<Move> GeneratePvLine() const;
	void ResetPvTable();

	int Depth, SelDepth;
	uint64_t Nodes;
	SearchStatistics Statistics;
	SearchConstraints Constraints;
	Histories History;
	std::chrono::high_resolution_clock::time_point StartSearchTime;
	std::array<std::array<uint64_t, 64>, 64> RootNodeCounts;
	std::unique_ptr<std::array<AccumulatorRepresentation, MaxDepth + 1>> Accumulators;

	std::array<std::array<Move, MaxDepth + 1>, MaxDepth + 1> PvTable;
	std::array<int, MaxDepth + 1> PvLength;

	std::array<std::array<int, 32>, 32> LMRTable;

	// Reused variables / stack
	std::array<MoveList, MaxDepth> MoveListStack{};
	std::array<int, MaxDepth> StaticEvalStack;
	std::array<int, MaxDepth> EvalStack;
	std::array<int, MaxDepth> CutoffCount;
	std::array<int, MaxDepth> DoubleExtensions;
	std::array<Move, MaxDepth> ExcludedMoves;

};
