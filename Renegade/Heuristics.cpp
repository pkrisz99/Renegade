#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
}

void Heuristics::AddEntry(unsigned __int64 hash, eval e, int scoreType) {
	//int usage = Hashes.capacity() * sizeof(T) + sizeof(vec);
	//std::cout << usage << std::endl;
	HashedEntryCount += 1;
	HashEntry entry;
	entry.score = e.score;
	entry.scoreType = scoreType;
	if (e.moves.size() > 0) {
		entry.moves = e.moves;
	}
	Hashes[hash] = entry;
}

std::tuple<bool, HashEntry> Heuristics::RetrieveEntry(unsigned __int64 hash) {
	// Using this changes the PV - no clue why
	if (Hashes.find(hash) != Hashes.end()) {
		HashEntry entry = Hashes[hash];
		return { true, entry };
	}
	return { false,  NoEntry };
}

void Heuristics::ClearEntries() {
	Hashes.clear();
	HashedEntryCount = 0;

	KillerMoves.clear();
	KillerMoves.reserve(100);
	for (int i = 0; i < 100; i++) {
		std::array<Move, 2> a = std::array<Move, 2>();
		a[0] = Move(0, 0);
		a[1] = Move(0, 0);
		KillerMoves.push_back(a);
	}
}

void Heuristics::ClearPv() {
	PvMoves.clear();
}

void Heuristics::SetPv(std::vector<Move> pv) {
	PvMoves = pv;
}

void Heuristics::AddKillerMove(Move move, int level) {
	if (IsKillerMove(move, level)) return;
	KillerMoves[level][1] = KillerMoves[level][0];
	KillerMoves[level][0] = move;
}

bool Heuristics::IsKillerMove(Move move, int level) {
	if ((KillerMoves[level][0].from == move.from) && (KillerMoves[level][0].to == move.to)) return true;
	if ((KillerMoves[level][1].from == move.from) && (KillerMoves[level][1].to == move.to)) return true;
	return false;
}

bool Heuristics::IsPvMove(Move move, int level) {
	if (level > PvMoves.size()) return false;
	if ((move.from == PvMoves[level-1].from) && (move.to == PvMoves[level-1].to)) return true;
	return false;
}