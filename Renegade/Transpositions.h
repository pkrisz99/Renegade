#pragma once
#include "Move.h"
#include "Utils.h"
#include <algorithm>
#include <array>
#include <memory>

namespace ScoreType {
	constexpr int Invalid = 0;
	constexpr int Exact = 1;
	constexpr int UpperBound = 2; // Alpha (fail-low)
	constexpr int LowerBound = 3; // Beta (fail-high)
};

struct TranspositionEntry {
	uint32_t hash = 0;
	int16_t score = 0;
	int16_t rawEval = 0;
	uint16_t quality = 0;
	uint8_t depth = 0;
	uint8_t scoreType = ScoreType::Invalid;
	uint16_t packedMove = 0;
	bool ttPv = false;

	inline bool IsCutoffPermitted(const int searchDepth, const int alpha, const int beta) const {
		if (searchDepth > depth) return false;
		return scoreType == ScoreType::Exact
			|| (scoreType == ScoreType::UpperBound && score <= alpha)
			|| (scoreType == ScoreType::LowerBound && score >= beta);
	}
};
struct alignas(64) TranspositionCluster {
	std::array<TranspositionEntry, 4> entries{};
};

static_assert(sizeof(TranspositionEntry) == 16);
static_assert(sizeof(TranspositionCluster) == 64);

class Transpositions
{
public:
	Transpositions();
	void Store(const uint64_t hash, const int depth, const int16_t score, const int scoreType, const int16_t rawEval, const Move& bestMove, const int level, const bool ttPv);
	bool Probe(const uint64_t hash, TranspositionEntry& entry, const int level) const;
	void Prefetch(const uint64_t hash) const;
	void IncreaseAge();
	void SetSize(const int megabytes);
	void Clear();
	int GetHashfull() const;

private:
	std::vector<TranspositionCluster> Table;
	uint64_t HashMask;
	uint16_t Age;

	inline uint64_t GetClusterIndex(const uint64_t hash) const {
		return hash & HashMask;
	}

	inline uint32_t GetStoredHash(const uint64_t hash) const {
		return static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);
	}
};

