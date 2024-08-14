#pragma once
#include "Move.h"
#include "Utils.h"
#include <algorithm>
#include <array>
#include <memory>

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

	inline bool IsCutoffPermitted(const int searchDepth, const int alpha, const int beta) const {
		if (searchDepth > depth) return false;
		return (scoreType == ScoreType::Exact)
			|| ((scoreType == ScoreType::UpperBound) && (score <= alpha))
			|| ((scoreType == ScoreType::LowerBound) && (score >= beta));
	}
};
static_assert(sizeof(TranspositionEntry) == 16);

class Transpositions
{
public:
	Transpositions();
	void Store(const uint64_t hash, const int depth, int score, const int scoreType, const Move& bestMove, const int level);
	bool Probe(const uint64_t& hash, TranspositionEntry& entry, const int level) const;
	void Prefetch(const uint64_t hash) const;
	void IncreaseAge();
	void SetSize(const int megabytes);
	void Clear();
	int GetHashfull() const;

private:
	std::vector<TranspositionEntry> Table;
	uint64_t HashFilter;
	uint16_t Age;
};

