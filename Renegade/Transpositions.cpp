#include "Transpositions.h"

Transpositions::Transpositions() {
	SetSize(1); // set an initial size, it will be resized before using
}

void Transpositions::Store(const uint64_t hash, const int depth, const int16_t score, const int scoreType, const int16_t rawEval, const Move& bestMove, const int level, const bool ttPv) {

	assert(std::abs(score) > MateEval);
	assert(HashMask != 0);
	if (std::abs(score) > MateEval) return;

	const uint64_t key = hash & HashMask;
	const uint16_t quality = Age * 2 + depth;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);
	const TranspositionCluster& cluster = Table[key];

	// Find the slot to use
	const int candidateSlot = [&] {
		int currentWorst = -1, currentWorstQuality = 1000000;
		for (int i = 0; i < cluster.Entries.size(); i++) {
			if (cluster.Entries[i].scoreType == ScoreType::Invalid) return i;
			if (cluster.Entries[i].hash == storedHash) return i;

			const int quality = cluster.Entries[i].quality;
			if (quality < currentWorstQuality) {
				currentWorst = i;
				currentWorstQuality = quality;
			};
		};
		assert(currentWorst != -1);
		return currentWorst;
	}();

	TranspositionEntry& candidateEntry = Table[key].Entries[candidateSlot];

	// Check if replaceable

	if (quality >= candidateEntry.quality) {
		candidateEntry.depth = depth;
		if (candidateEntry.hash != storedHash || !bestMove.IsNull()) {
			candidateEntry.packedMove = bestMove.Pack();
		}
		candidateEntry.scoreType = scoreType;
		candidateEntry.hash = storedHash;
		candidateEntry.quality = quality;
		candidateEntry.rawEval = rawEval;
		candidateEntry.ttPv = ttPv;

		candidateEntry.score = [&] {
			if (IsWinningMateScore(score)) return static_cast<int16_t>(score + level);
			if (IsLosingMateScore(score)) return static_cast<int16_t>(score - level);
			return score;
		}();
	}
}

bool Transpositions::Probe(const uint64_t hash, TranspositionEntry& returned, const int level) const {
	assert(HashMask != 0);
	const uint64_t key = GetClusterIndex(hash);
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);
	const TranspositionCluster& cluster = Table[key];

	for (const TranspositionEntry& entry : cluster.Entries) {
		if (entry.hash != storedHash) continue;

		returned = entry;
		returned.score = [&] {
			const int32_t score = entry.score;
			if (IsLosingMateScore(score)) return score + level;
			if (IsWinningMateScore(score)) return score - level;
			return score;
		}();
		return true;
	}
	return false;
}

void Transpositions::Prefetch(const uint64_t hash) const {
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
	const uint64_t key = GetClusterIndex(hash);
	__builtin_prefetch(&Table[key]);
#endif
}

void Transpositions::IncreaseAge() {
	if (Age < 32000) Age += 1;
}

void Transpositions::SetSize(const int megabytes) {
	assert(megabytes > 0);
	const uint64_t theoreticalClusterCount = static_cast<uint64_t>(megabytes) * 1024 * 1024 / sizeof(TranspositionCluster);
	const uint64_t actualClusterCount = std::bit_floor(theoreticalClusterCount);
	HashMask = actualClusterCount - 1;
	Clear();
}

void Transpositions::Clear() {
	Table.clear();
	Table.reserve(HashMask + 1);
	for (uint64_t i = 0; i < HashMask + 1; i++) Table.push_back(TranspositionCluster());
	Table.shrink_to_fit(); // I don't think that's needed
	Age = 0;
}

int Transpositions::GetHashfull() const {
	// Approximate by checking the usage of the first 1000 clusters
	int hashfull = 0;
	for (int i = 0; i < 1000; i++) {
		for (const TranspositionEntry& entry : Table[i].Entries) {
			if (entry.quality >= Age * 2) hashfull += 1;
		}
	}
	return hashfull / 4;
}
