#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	KillerMoves.reserve(100);
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
	for (auto v : KillerMoves) {
		v[0] = Move(0, 0);
		v[1] = Move(0, 0);
	}
}

void Heuristics::AddKillerMove(Move move, int level) {
	KillerMoves[level][1] = KillerMoves[level][0];
	KillerMoves[level][0] = move;
}

bool Heuristics::IsKillerMove(Move move, int level) {
	if ((KillerMoves[level][0].from == move.from) && (KillerMoves[level][0].to == move.to)) return true;
	if ((KillerMoves[level][1].from == move.from) && (KillerMoves[level][1].to == move.to)) return true;
	return false;
}