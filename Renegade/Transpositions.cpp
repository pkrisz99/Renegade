#include "Transpositions.h"

Transpositions::Transpositions() {
	SetSize(1, 1); // set an initial size, it will be resized before using
}

Transpositions::~Transpositions() {
	FreeTable();
}

void Transpositions::Store(const uint64_t hash, const int depth, const int16_t score, const int scoreType, const int16_t rawEval, const Move& bestMove, const int level, const bool ttPv) {

	assert(std::abs(score) < MateEval);
	assert(TableSize > 1);

	const uint64_t key = GetClusterIndex(hash);
	const uint32_t storedHash = GetStoredHash(hash);
	const TranspositionCluster& cluster = Table[key];

	// Find the slot to use
	const int candidateSlot = [&]() -> int {
		int currentWorst = -1;
		int currentWorstQuality = std::numeric_limits<int>::max();

		for (size_t i = 0; i < cluster.entries.size(); i++) {
			const TranspositionEntry& entry = cluster.entries[i];
			if (entry.scoreType == ScoreType::Invalid) return i;
			if (entry.hash == storedHash) return i;

			const int entryQuality = RecordingQuality(entry.generation, entry.depth);
			if (entryQuality < currentWorstQuality) {
				currentWorst = i;
				currentWorstQuality = entryQuality;
			};
		};
		assert(currentWorst != -1);
		return currentWorst;
	}();

	TranspositionEntry& candidateEntry = Table[key].entries[candidateSlot];

	// Check if the candidate entry is replaceable
	const bool replaceable = [&] {
		if (storedHash != candidateEntry.hash) return true;
		if (scoreType == ScoreType::Exact) return true;
		return depth + 3 + ttPv * 2 >= candidateEntry.depth;
	}();

	// Update the transposition entry
	if (replaceable) {
		candidateEntry.depth = depth;
		if (candidateEntry.hash != storedHash || !bestMove.IsNull()) {
			candidateEntry.packedMove = bestMove.Pack();
		}
		candidateEntry.scoreType = scoreType;
		candidateEntry.hash = storedHash;
		candidateEntry.generation = CurrentGeneration;
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
	assert(TableSize > 1);
	const uint64_t key = GetClusterIndex(hash);
	const uint32_t storedHash = GetStoredHash(hash);
	const TranspositionCluster& cluster = Table[key];

	for (const TranspositionEntry& entry : cluster.entries) {
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

void Transpositions::AllocateTable(const uint64_t clusterCount) {
	// Allocate memory
#if defined(_MSC_VER) || defined(_WIN32)
	Table = static_cast<TranspositionCluster*>(_aligned_malloc(clusterCount * sizeof(TranspositionCluster), 64));
#else
	Table = static_cast<TranspositionCluster*>(std::aligned_alloc(64, clusterCount * sizeof(TranspositionCluster)));
#endif

	// Request huge pages if available
#if defined(MADV_HUGEPAGE)
	const int r = madvise(Table, clusterCount * sizeof(TranspositionCluster), MADV_HUGEPAGE);
	cout << "info string madvise called: " << r << endl;
	cout << "info string madvise args: " << Table << " " << (clusterCount * sizeof(TranspositionCluster)) << endl;
	if (r == -1) {
		perror("madvise error");
	}
#endif

	// Set cluter count
	TableSize = clusterCount;
}

void Transpositions::FreeTable() {
#if defined(_MSC_VER) || defined(_WIN32)
	if (Table != nullptr) _aligned_free(Table);
#else
	if (Table != nullptr) std::free(Table);
#endif
	TableSize = 0;
}

void Transpositions::IncreaseAge() {
	if (CurrentGeneration < 65000) CurrentGeneration += 1;
}

void Transpositions::SetSize(const int megabytes, const int threadCount) {
	assert(megabytes > 0);
	const uint64_t theoreticalClusterCount = static_cast<uint64_t>(megabytes) * 1024 * 1024 / sizeof(TranspositionCluster);
	const uint64_t actualClusterCount = std::bit_floor(theoreticalClusterCount);

	if (TableSize != actualClusterCount) {
		FreeTable();
		AllocateTable(actualClusterCount);
	}
	Clear(threadCount);
}

void Transpositions::Clear(const int threadCount) {
	// Clear entries, potentially in parallel, with as many threads as set to do search
	// This speeds up initialization significantly for large hash sizes

	const uint64_t chunkSize = (TableSize + threadCount - 1) / threadCount; // ceil division
	std::vector<std::thread> threads;

	for (int i = 0; i < threadCount; i++) {
		threads.emplace_back([this, i, chunkSize]() {
			const uint64_t startIndex = chunkSize * i;
			const uint64_t endIndex = std::min(startIndex + chunkSize, TableSize);
			const uint64_t length = endIndex - startIndex;
			std::memset(&Table[startIndex], 0, length * sizeof(TranspositionCluster));
		});
	}
	for (auto& t : threads) t.join();

	CurrentGeneration = 0;
}

int Transpositions::GetHashfull() const {
	// Approximate by checking the usage of the first 1000 clusters
	int hashfull = 0;
	for (int i = 0; i < 1000; i++) {
		for (const TranspositionEntry& entry : Table[i].entries) {
			if (RecordingQuality(entry.generation, entry.depth) >= RecordingQuality(CurrentGeneration, 0)) hashfull += 1;
		}
	}
	return hashfull / 4;
}
