#pragma once
#include "Histories.h"
#include "Movepicker.h"
#include "Neural.h"
#include "Position.h"
#include "Reporting.h"
#include "Transpositions.h"
#include "Utils.h"
#include <atomic>
#include <iomanip>
#include <algorithm>
#include <condition_variable>
#include <fstream>
//#include <format>
#include <list>
#include <mutex>
#include <random>
#include <thread>
#include <tuple>

/*
* This is the code responsible for performing move selection.
* SearchRecursive() is the main alpha-beta search, and SearchQuiescence() is called in leaf nodes.
*/

enum class ThreadAction { Sleep, Search, Exit };

class alignas(64) ThreadData {
public:
	void ResetStatistics();

	int RootDepth = 0, SelDepth = 0;
	int64_t Nodes = 0;
	Histories History;
	MultiArray<Move, MaxDepth + 1, MaxDepth + 1> PvTable;
	std::array<int, MaxDepth + 1> PvLength;
	EvaluationState EvalState;
	MultiArray<uint64_t, 64, 64> RootNodeCounts;

	// PV table
	void UpdatePvTable(const Move& move, const int level);
	void InitPvLength(const int level);
	std::vector<Move> GeneratePvLine() const;
	void ResetPvTable();

	// Reused variables / stack
	std::array<int, MaxDepth> StaticEvalStack;
	std::array<int, MaxDepth> EvalStack;
	std::array<int, MaxDepth> CutoffCount;
	std::array<Move, MaxDepth> ExcludedMoves;
	MultiArray<MovePicker, MaxDepth, 2> MovePickerStack{};

	Position CurrentPosition;

	std::thread Thread;
	int threadId;
	Results result;
	bool singlethreaded = false;

	inline bool IsMainThread() const {
		return threadId == 0;
	}

	// Thread handling
	std::mutex Mutex;
	std::condition_variable CondVar;
	ThreadAction Action;
	bool Exited = false;
};

class Search
{
public:
	Search();
	void ResetState(const bool clearTT);

	void StartThreads(const int threadCount);
	void StopThreads();
	void SetThreadCount(const int threadCount);
	void StartSearch(Position& position, const SearchParams params);
	void StopSearch();
	void Loop(ThreadData& t);
	Results SearchSinglethreaded(const Position& pos, const SearchParams& params);
	void WaitUntilReady();
	void Perft(Position& position, const int depth, const PerftType type) const;

#ifdef RENEGADE_DATAGEN
	static constexpr bool DatagenMode = true;
#else
	static constexpr bool DatagenMode = false;
#endif

	std::atomic<bool> Aborting = true;
	Transpositions TranspositionTable;

	std::list<ThreadData> Threads;
	std::atomic<int> ActiveThreadCount = 0;
	std::atomic<int> LoadedThreadCount = 0;

private:
	Results AggregateThreadResults() const;

	void SearchMoves(ThreadData& t);
	template<bool pvNode> int SearchRecursive(ThreadData& t, int depth, const int level, int alpha, int beta, const bool cutNode);
	template<bool pvNode> int SearchQuiescence(ThreadData& t, const int level, int alpha, int beta);

	int16_t Evaluate(ThreadData& t, const Position& position);
	uint64_t PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const;
	SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	bool ShouldAbort(const ThreadData& t);
	int DrawEvaluation(const ThreadData& t) const;

	
	SearchConstraints Constraints;
	std::chrono::high_resolution_clock::time_point StartSearchTime;
	MultiArray<int, 32, 32> LMRTable;

};
