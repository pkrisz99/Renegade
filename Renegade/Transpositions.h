#pragma once
#include "Move.h"
#include "Utils.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <memory>
#include <thread>

#ifdef __linux__
#include <sys/mman.h>
#endif

namespace ScoreType {
	constexpr int Invalid = 0;
	constexpr int Exact = 1;
	constexpr int UpperBound = 2; // Alpha (fail-low)
	constexpr int LowerBound = 3; // Beta (fail-high)
};

struct TranspositionEntry {
	uint32_t hash;
	int16_t score;
	int16_t rawEval;
	uint16_t generation;
	uint8_t depth;
	uint8_t scoreType;
	uint16_t packedMove;
	bool ttPv;

	inline bool IsCutoffPermitted(const int searchDepth, const int alpha, const int beta) const {
		if (searchDepth > depth) return false;
		return scoreType == ScoreType::Exact
			|| (scoreType == ScoreType::UpperBound && score <= alpha)
			|| (scoreType == ScoreType::LowerBound && score >= beta);
	}
};
struct alignas(64) TranspositionCluster {
	std::array<TranspositionEntry, 4> entries;
};

static_assert(sizeof(TranspositionEntry) == 16);
static_assert(sizeof(TranspositionCluster) == 64);

class Transpositions
{
public:
	Transpositions();
	~Transpositions();
	void Store(const uint64_t hash, const int depth, const int16_t score, const int scoreType, const int16_t rawEval, const Move& bestMove, const int level, const bool ttPv);
	bool Probe(const uint64_t hash, TranspositionEntry& entry, const int level) const;
	void Prefetch(const uint64_t hash) const;
	void IncreaseAge();
	void SetSize(const int megabytes, const int threadCount);
	void Clear(const int threadCount);
	int GetHashfull() const;

private:
	TranspositionCluster* Table = nullptr;
	uint64_t TableSize = 0;
	uint16_t CurrentGeneration;

	// Handles direct memory allocation and freeing, required for multiplatform and performance reasons:
	void AllocateTable(const uint64_t clusterCount);
	void FreeTable();

	inline uint64_t GetClusterIndex(const uint64_t hash) const {
		assert((TableSize & (TableSize - 1)) == 0);
		return hash & (TableSize - 1);
	}

	inline uint32_t GetStoredHash(const uint64_t hash) const {
		return static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);
	}

	inline int RecordingQuality(const int generation, const int depth) const {
		return generation * 4 + depth;
	}
};

