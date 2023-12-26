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
#include <unordered_map>

/*
* This is the code responsible for performing move selection.
* SearchRecursive() is the main alpha-beta search, and SearchQuiescence() is called in leaf nodes.
* It also handles opening books and communicating the interim search results.
*/

class Search
{
public:
	Search();
	void ResetState(const bool clearTT);
	void SetTunableSettings(std::unordered_map<std::string, int>& params);

	void Perft(Board& board, const int depth, const PerftType type) const;
	const Results SearchMoves(Board board, const SearchParams params, const EngineSettings settings, const bool display);
	bool StaticExchangeEval(const Board& board, const Move& move, const int threshold) const;

	bool Aborting = false;
	bool DatagenMode = false;
	Heuristics Heuristics;

private:
	int SearchRecursive(Board& board, int depth, const int level, int alpha, int beta, const bool canNullMove);
	int SearchQuiescence(Board& board, const int level, int alpha, int beta);
	int Evaluate(const Board& board);
	uint64_t PerftRecursive(Board& board, const int depth, const int originalDepth, const PerftType type) const;
	const SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	inline bool ShouldAbort() const;
	int DrawEvaluation();
	void ResetStatistics();

	// NNUE
	void SetupAccumulators(const Board& board);
	template <bool push> void UpdateAccumulators(const Move& m, const uint8_t movedPiece, const uint8_t capturedPiece);

	// Communication
	void PrintInfo(const Results& e, const EngineSettings& settings) const;
	void PrintPretty(const Results& e, const EngineSettings& settings) const;
	void PrintBestmove(const Move& move) const;

	AccumulatorRepresentation Accumulator;

	std::chrono::high_resolution_clock::time_point StartSearchTime;
	uint16_t Age = 0;

	int Depth;
	SearchStatistics Statistics;
	SearchConstraints Constraints;

	// Reused variables / stack
	std::vector<Move> MoveList;
	std::array<std::vector<std::tuple<Move, int>>, MaxDepth> MoveOrder;
	std::array<Board, MaxDepth> Boards;
	std::array<int, MaxDepth> EvalStack;
	std::array<MoveAndPiece, MaxDepth> MoveStack;

	// Tuning
	int lmr_depth;
	double lmr_multip;
	double lmr_base;
	int lmr_moves_pv;
	int lmr_moves_nonpv;
	int iir_depth;
	int rfp_depth;
	int rfp_margin_1;
	int rfp_margin_after;
	int fp_depth;
	int fp_margin_1;
	int fp_margin_after;
	int lmp_depth;
	int lmp_base;
	int nmp_depth;
	int nmp_base;
	int nmp_scale_eval;
	int nmp_scale_depth;
	int nmp_scale_eval_cap;
	int see_depth;


	std::array<std::array<int, 32>, 32> LMRTable;

};

