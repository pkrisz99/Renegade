#include "Transpositions.h"

Transpositions::Transpositions() {
	SetSize(1); // set an initial size, it will be resized before using
}

void Transpositions::Store(const uint64_t hash, const int depth, const int16_t score, const int scoreType, const int16_t rawEval, const Move& bestMove, const int level, const bool ttPv) {

	//assert(std::abs(score) > MateEval);
	assert(HashMask != 0);
	if (std::abs(score) > MateEval) return;

	const uint64_t key = hash & HashMask;
	const uint16_t quality = Age * 2 + depth;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);

	if (quality >= Table[key].quality) { // (TranspositionTable[key].depth <= depth)
		Table[key].depth = depth;
		if (Table[key].hash != storedHash || !bestMove.IsNull()) {
			Table[key].packedMove = bestMove.Pack();
		}
		Table[key].scoreType = scoreType;
		Table[key].hash = storedHash;
		Table[key].quality = quality;
		Table[key].rawEval = rawEval;
		Table[key].ttPv = ttPv;

		Table[key].score = [&] {
			if (IsWinningMateScore(score)) return static_cast<int16_t>(score + level);
			if (IsLosingMateScore(score)) return static_cast<int16_t>(score - level);
			return score;
		}();
	}
}

bool Transpositions::Probe(const uint64_t& hash, TranspositionEntry& entry, const int level) const {
	assert(HashMask != 0);
	const uint64_t key = hash & HashMask;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);

	if (Table[key].hash == storedHash) {
		entry = Table[key];

		entry.score = [&] {
			const int32_t score = Table[key].score;
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
	const uint64_t key = hash & HashMask;
	__builtin_prefetch(&Table[key]);
#endif
}

void Transpositions::IncreaseAge() {
	if (Age < 32000) Age += 1;
}

void Transpositions::SetSize(const int megabytes) {
	assert(megabytes != 0);
	const uint64_t theoreticalEntryCount = static_cast<uint64_t>(megabytes) * 1024 * 1024 / sizeof(TranspositionEntry);
	int bits = 0;
	while ((1ull << bits) <= theoreticalEntryCount) bits += 1;
	bits -= 1;
	HashMask = (1ull << bits) - 1;
	Clear();
}

void Transpositions::Clear() {
	Table.clear();
	Table.reserve(HashMask + 1);
	for (uint64_t i = 0; i < HashMask + 1; i++) Table.push_back(TranspositionEntry());
	Table.shrink_to_fit();
	Age = 0;
}

int Transpositions::GetHashfull() const {
	// Approximate by checking the usage of the first 1000 entries
	int hashfull = 0;
	for (int i = 0; i < 1000; i++) {
		const TranspositionEntry& entry = Table[i];
		if (Table[i].quality >= Age * 2) hashfull += 1;
	}
	return hashfull;
}
