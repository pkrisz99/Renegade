#pragma once
#include "Board.h"
#include "Utils.h"
#include <algorithm>
#include <array>
#include <memory>

/*
* Collection of functions to make search more efficient.
* Handles the transposition table, history heuristic, killer moves, and the PV table.
*/

namespace ScoreType {
	constexpr int Exact = 0;
	constexpr int UpperBound = 1; // Alpha (fail-low)
	constexpr int LowerBound = 2; // Beta (fail-high)
};

struct TranspositionEntry {
	uint32_t hash = 0;
	int32_t score = 0;
	uint16_t quality = 0;
	uint8_t depth = 0;
	uint8_t scoreType = 0;
	uint16_t packedMove = 0;
	
	bool IsCutoffPermitted(const int searchDepth, const int alpha, const int beta) const;
};

static_assert(sizeof(TranspositionEntry) == 16);

struct MovePicker {
	MoveList& moveList;
	int index;

	MovePicker(MoveList& scoredMoveList) : moveList(scoredMoveList) {
		index = 0;
	}

	std::pair<Move, int> get() {
		assert(hasNext());

		int bestOrderScore = moveList[index].orderScore;
		int bestIndex = index;

		for (int i = index + 1; i < moveList.size(); i++) {
			if (moveList[i].orderScore > bestOrderScore) {
				bestOrderScore = moveList[i].orderScore;
				bestIndex = i;
			}
		}

		std::swap(moveList[bestIndex], moveList[index]);
		index += 1;
		return { moveList[index - 1].move, moveList[index - 1].orderScore };
	}

	bool hasNext() const {
		return index < moveList.size();
	}
};

class Heuristics
{
public:

	Heuristics();
	~Heuristics();
	[[nodiscard]] int CalculateOrderScore(const Board& board, const Move& m, const int level, const Move& ttMove,
		const std::array<MoveAndPiece, MaxDepth>& moveStack, const bool losingCapture, const bool useMoveStack, const uint64_t opponentAttacks) const;
	
	// PV table
	void UpdatePvTable(const Move& move, const int level);
	void InitPvLength(const int level);
	void GeneratePvLine(std::vector<Move>& list) const;
	void ResetPvTable();

	// Killer and countermoves
	void AddKillerMove(const Move& m, const int level);
	[[nodiscard]] bool IsCountermove(const Move& previousMove, const Move& thisMove) const;
	[[nodiscard]] bool IsKillerMove(const Move& move, const int level) const;
	[[nodiscard]] bool IsFirstKillerMove(const Move& move, const int level) const;
	[[nodiscard]] bool IsSecondKillerMove(const Move& move, const int level) const;
	void ClearKillerAndCounterMoves();
	void AddCountermove(const Move& previousMove, const Move& thisMove);

	// History heuristic
	void UpdateHistory(const Move& m, const int16_t delta, const uint8_t piece, const int depth, const std::array<MoveAndPiece, MaxDepth>& moveStack,
		const int level, const bool fromSquareAttacked, const bool toSquareAttacked);
	void ClearHistory();

	// Transposition table
	void AddTranspositionEntry(const uint64_t hash, const uint16_t age, const int depth, int score, const int scoreType, const Move& bestMove, const int level);
	bool RetrieveTranspositionEntry(const uint64_t& hash, TranspositionEntry& entry, const int level);
	void SetHashSize(const int megabytes);
	[[nodiscard]] int GetHashfull();
	void GetTranspositionInfo(uint64_t& ttTheoretical, uint64_t& ttUsable, uint64_t& ttBits, uint64_t& ttUsed);
	void ClearTranspositionTable();
	void PrefetchTranspositionEntry(const uint64_t hash) const;

	std::array<std::array<Move, MaxDepth + 1>, MaxDepth + 1> PvTable;
	std::array<int, MaxDepth + 1> PvLength;

	std::array<std::array<Move, 2>, MaxDepth> KillerMoves;


private:
	void UpdateHistoryValue(int16_t& value, const int amount);

	std::vector<TranspositionEntry> TranspositionTable;
	uint64_t HashFilter;
	int HashBits;
	uint64_t TranspositionEntryCount;
	uint64_t TheoreticalTranspositionEntires;

	std::array<std::array<std::array<std::array<int16_t, 64>, 14>, 2>, 2> HistoryTables;
	std::array<std::array<Move, 64>, 64> CounterMoves;

	using Continuations = std::array<std::array<std::array<std::array<int16_t, 64>, 14>, 64>, 14>;
	Continuations* ContinuationHistory;
};

