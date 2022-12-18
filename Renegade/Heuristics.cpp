#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	ApproxHashSize = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
}

void Heuristics::AddEntry(unsigned __int64 hash, int score, int scoreType) {
	if (ApproxHashSize + sizeof(HashEntry) >= MaximumHashSize) return;
	HashedEntryCount += 1;
	HashEntry entry;
	entry.score = score;
	entry.scoreType = scoreType;
	Hashes[hash] = entry;
	ApproxHashSize += sizeof(HashEntry); // Is this good?
}

std::tuple<bool, HashEntry> Heuristics::RetrieveEntry(unsigned __int64 hash) {

	if (Hashes.find(hash) != Hashes.end()) {
		HashEntry entry = Hashes[hash];
		return { true, entry };
	}
	return { false,  NoEntry };
}

void Heuristics::ClearEntries() {
	Hashes.clear();
	HashedEntryCount = 0;
	ApproxHashSize = 0;

	KillerMoves.clear();
	KillerMoves.reserve(100);
	for (int i = 0; i < 100; i++) {
		std::array<Move, 2> a = std::array<Move, 2>();
		a[0] = Move(0, 0);
		a[1] = Move(0, 0);
		KillerMoves.push_back(a);
	}
	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 30; j++) PvTable[i][j] = Move();
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

void Heuristics::SetHashSize(int megabytes) {
	MaximumHashSize = megabytes * 1024ULL * 1024ULL;
}

int Heuristics::GetHashfull() {
	if (MaximumHashSize <= 0) return -1;
	return (int)(ApproxHashSize * 1000ULL / MaximumHashSize);
}

void Heuristics::UpdatePvTable(Move move, int level) {
	PvTable[level][level] = move;
	for (int i = level + 1; i < 20; i++) {
		Move lowerMove = PvTable[level + 1][i];
		if ((lowerMove.from == 0) && (lowerMove.to == 0)) break;
		PvTable[level][i] = lowerMove;
	}
}

std::vector<Move> Heuristics::GetPvLine() {
	std::vector<Move> list = std::vector<Move>();
	for (int i = 1; i < 20; i++) {
		Move m = PvTable[1][i];
		if ((m.from == 0) && (m.to == 0)) break;
		list.push_back(m);
	}
	return list;
}