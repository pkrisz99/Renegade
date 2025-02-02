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

class alignas(64) ThreadData {
public:
	void ResetStatistics();

	int Depth = 0, SelDepth = 0;
	uint64_t Nodes = 0;
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
	std::array<MoveList, MaxDepth> MoveListStack{};
	std::array<int, MaxDepth> StaticEvalStack;
	std::array<int, MaxDepth> EvalStack;
	std::array<int, MaxDepth> CutoffCount;
	std::array<Move, MaxDepth> ExcludedMoves;
	std::array<bool, MaxDepth> SuperSingular;

	Position CurrentPosition;

	std::thread Thread;
	int threadId;
	Results result;
	bool singlethreaded = false;

	inline bool IsMainThread() const {
		return threadId == 0;
	}

	struct LoopingLogic {
	public:
		inline void Step() {
			while (!Ready.load()) {};
			Passthrough.store(true);
			CondVar.notify_all();
		}

		inline void Exit() {
			Exiting.store(true);
			Passthrough.store(true);
			CondVar.notify_all();
		}

		inline void Wait() {
			Passthrough.store(false);
			Ready.store(true);
			CondVar.notify_all();

			std::unique_lock lock(Mutex);
			CondVar.wait(lock, [&] {
				return Passthrough.load();
			});
		}

		inline bool IsExiting() const {
			return Exiting.load();
		}
		std::atomic<bool> Ready = false;  // in very rare cases prevents the engine from locking up due to unfortunate timing of changing Passthrough (?)

	private:
		std::mutex Mutex;
		std::condition_variable CondVar;
		std::atomic<bool> Passthrough = false;
		std::atomic<bool> Exiting = false;
	};

	LoopingLogic Looping;
};

class Search
{
public:
	Search();
	void ResetState(const bool clearTT);

	void StartThreads(const int threadCount);
	void StopThreads();
	void SetThreadCount(const int threadCount);
	void StartSearch(Position& position, const SearchParams params, const bool display);
	void StopSearch();
	void Loop(ThreadData& t);
	Results SearchSinglethreaded(const Position& pos, const SearchParams& params);
	void WaitUntilReady() const;

	void Perft(Position& position, const int depth, const PerftType type) const;

	bool StaticExchangeEval(const Position& position, const Move& move, const int threshold) const;

	std::atomic<bool> Aborting = true;
	bool DatagenMode = false;
	Transpositions TranspositionTable;

	std::list<ThreadData> Threads;
	std::atomic<int> ActiveThreadCount = 0;
	std::atomic<int> LoadedThreadCount = 0;

private:
	Results AggregateThreadResults() const;

	void SearchMoves(ThreadData& t);
	int SearchRecursive(ThreadData& t, int depth, const int level, int alpha, int beta, const bool pvNode, const bool cutNode);
	int SearchQuiescence(ThreadData& t, const int level, int alpha, int beta, const bool pvNode);

	int16_t Evaluate(ThreadData& t, const Position& position, const int level);
	uint64_t PerftRecursive(Position& position, const int depth, const int originalDepth, const PerftType type) const;
	SearchConstraints CalculateConstraints(const SearchParams params, const bool turn) const;
	bool ShouldAbort(const ThreadData& t);
	int DrawEvaluation(const ThreadData& t) const;

	void OrderMoves(const ThreadData& t, const Position& position, MoveList& ml, const int level, const Move& ttMove);
	void OrderMovesQ(const ThreadData& t, const Position& position, MoveList& ml, const int level, const Move& ttMove);
	int CalculateOrderScore(const ThreadData& t, const Position& position, const Move& m, const int level, const Move& ttMove,
		const bool losingCapture, const bool useMoveStack) const;

	
	SearchConstraints Constraints;
	std::chrono::high_resolution_clock::time_point StartSearchTime;
	MultiArray<int, 32, 32> LMRTable;

};
