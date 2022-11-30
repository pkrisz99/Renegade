#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
}

void Heuristics::AddEntry(unsigned __int64 hash, eval e, int level) {
	//int usage = Hashes.capacity() * sizeof(T) + sizeof(vec);
	//std::cout << usage << std::endl;
	HashedEntryCount += 1;
	HashEntry entry;
	entry.score = e.score;
	if (e.moves.size() > 0) {
		entry.moves = e.moves;
	}
	Hashes[hash ^ (PlyHash * level)] = entry;
}

std::tuple<bool, HashEntry>  Heuristics::RetrieveEntry(unsigned __int64 hash, int level) {
	unsigned __int64 key = hash ^ (PlyHash * level);
	if (Hashes.find(key) != Hashes.end()) {
		return { true, Hashes[key] };
	}
	return { false, NoEntry };
}

void Heuristics::ClearEntries() {
	Hashes.clear();
	HashedEntryCount = 0;
}