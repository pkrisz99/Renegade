#include "Transpositions.h"

Transpositions::Transpositions() {
	SetSize(1); // set an initial size, it will be resized before using
}

void Transpositions::Store(const uint64_t hash, const int depth, int score, const int scoreType, const Move& bestMove, const int level) {

	//assert(std::abs(score) > MateEval);
	assert(HashFilter != 0);
	if (std::abs(score) > MateEval) return;

	const uint64_t key = hash & HashFilter;
	const uint16_t quality = Age * 2 + depth;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);

	if (quality >= Table[key].quality) { // (TranspositionTable[key].depth <= depth)
		if (Table[key].hash == 0) EntryCount += 1;
		Table[key].depth = depth;
		if (Table[key].hash != storedHash || !bestMove.IsNull()) {
			Table[key].packedMove = bestMove.Pack();
		}
		Table[key].scoreType = scoreType;
		Table[key].hash = storedHash;
		Table[key].quality = quality;

		Table[key].score = [&] {
			if (IsWinningMateScore(score)) return score + level;
			if (IsLosingMateScore(score)) return score - level;
			return score;
		}();
	}
}

bool Transpositions::Retrieve(const uint64_t& hash, TranspositionEntry& entry, const int level) const {
	assert(HashFilter != 0);
	const uint64_t key = hash & HashFilter;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);

	if (Table[key].hash == storedHash) {
		entry.depth = Table[key].depth;
		entry.packedMove = Table[key].packedMove;
		entry.scoreType = Table[key].scoreType;
		entry.quality = Table[key].quality; // not needed
		entry.hash = storedHash;

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
	const uint64_t key = hash & HashFilter;
	__builtin_prefetch(&Table[key]);
#endif
}

void Transpositions::IncreaseAge() {
	if (Age < 32000) Age += 1;
}

void Transpositions::SetSize(const int megabytes) {
	assert(megabytes != 0);
	EntryCount = 0;
	TheoreticalEntryCount = static_cast<uint64_t>(megabytes) * 1024 * 1024 / sizeof(TranspositionEntry);
	int bits = 0;
	while ((1ull << bits) <= TheoreticalEntryCount) bits += 1;
	bits -= 1;
	HashFilter = (1ull << bits) - 1;
	HashBits = bits;
	Clear();
}

void Transpositions::Clear() {
	Table.clear();
	Table.reserve(HashFilter + 1);
	for (uint64_t i = 0; i < HashFilter + 1; i++) Table.push_back(TranspositionEntry());
	Table.shrink_to_fit();
	EntryCount = 0;
	Age = 0;
}

int Transpositions::GetHashfull() const {
	return static_cast<int>(EntryCount * 1000 / (HashFilter + 1));
}

void Transpositions::GetInfo(uint64_t& ttTheoretical, uint64_t& ttUsable, uint64_t& ttBits, uint64_t& ttUsed) const {
	ttTheoretical = TheoreticalEntryCount;
	ttUsable = HashFilter + 1;
	ttBits = HashBits;
	ttUsed = EntryCount;
}

bool TranspositionEntry::IsCutoffPermitted(const int searchDepth, const int alpha, const int beta) const {
	if (searchDepth > depth) return false;

	return (scoreType == ScoreType::Exact)
		|| ((scoreType == ScoreType::UpperBound) && (score <= alpha))
		|| ((scoreType == ScoreType::LowerBound) && (score >= beta));
}