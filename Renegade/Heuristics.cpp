#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	ApproxHashSize = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
}

void Heuristics::AddEntry(const uint64_t hash, const int score, const int scoreType) {
	if (ApproxHashSize + sizeof(HashEntry) >= MaximumHashSize) return;
	HashedEntryCount += 1;
	HashEntry entry;
	entry.score = score;
	entry.scoreType = scoreType;
	Hashes[hash] = entry;
	ApproxHashSize += sizeof(HashEntry); // Is this good?
}

const std::tuple<bool, HashEntry> Heuristics::RetrieveEntry(const uint64_t hash) {

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

void Heuristics::SetPv(const std::vector<Move> pv) {
	PvMoves = pv;
}

void Heuristics::AddKillerMove(const Move move, const int level) {
	if (IsKillerMove(move, level)) return;
	KillerMoves[level][1] = KillerMoves[level][0];
	KillerMoves[level][0] = move;
}

const bool Heuristics::IsKillerMove(const Move move, const int level) {
	if ((KillerMoves[level][0].from == move.from) && (KillerMoves[level][0].to == move.to)) return true;
	if ((KillerMoves[level][1].from == move.from) && (KillerMoves[level][1].to == move.to)) return true;
	return false;
}

const bool Heuristics::IsPvMove(const Move move, const int level) {
	if (level >= PvMoves.size()) return false;
	if ((move.from == PvMoves[level].from) && (move.to == PvMoves[level].to)) return true;
	return false;
}

void Heuristics::SetHashSize(const int megabytes) {
	MaximumHashSize = megabytes * 1024ULL * 1024ULL;
}

const int Heuristics::GetHashfull() {
	if (MaximumHashSize <= 0) return -1;
	return (int)(ApproxHashSize * 1000ULL / MaximumHashSize);
}

void Heuristics::UpdatePvTable(const Move move, const int level) {
	PvTable[level][level] = move;
	for (int i = level + 1; i < 20; i++) {
		Move lowerMove = PvTable[level + 1][i];
		if ((lowerMove.from == 0) && (lowerMove.to == 0)) break;
		PvTable[level][i] = lowerMove;
	}
}

const std::vector<Move> Heuristics::GetPvLine() {
	std::vector<Move> list = std::vector<Move>();
	for (int i = 0; i < 20; i++) {
		Move m = PvTable[0][i];
		if ((m.from == 0) && (m.to == 0)) break;
		list.push_back(m);
	}
	return list;
}

const int Heuristics::CalculateMoveOrderScore(Board board, const Move m, const int level, const float phase) {
	int orderScore = 0;
	const int attackingPiece = TypeOfPiece(board.GetPieceAt(m.from));
	const int attackedPiece = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 10000 };
	if (attackedPiece != PieceType::None) {
		orderScore = values[attackedPiece] - values[attackingPiece] + 10000;
	}

	if (m.flag == MoveFlag::PromotionToQueen) orderScore += 900; // Weights::QueenValue;
	else if (m.flag == MoveFlag::PromotionToRook) orderScore += 500; //Weights::RookValue;
	else if (m.flag == MoveFlag::PromotionToBishop) orderScore += 300; // Weights::BishopValue;
	else if (m.flag == MoveFlag::PromotionToKnight) orderScore += 300; // Weights::KnightValue;
	else if (m.flag == MoveFlag::EnPassantPerformed) orderScore += 100;

	bool turn = board.Turn;
	if (turn == Turn::White) {
		orderScore -= LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, m.from)], Weights[IndexLatePSQT(attackingPiece, m.from)], phase);
		orderScore += LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, m.to)], Weights[IndexLatePSQT(attackingPiece, m.to)], phase);
	}
	else {
		orderScore -= LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, Mirror[m.from])], Weights[IndexLatePSQT(attackingPiece, Mirror[m.from])], phase);
		orderScore += LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, Mirror[m.to])], Weights[IndexLatePSQT(attackingPiece, Mirror[m.to])], phase);
	}

	if (IsKillerMove(m, level)) orderScore += 200000;
	if (IsPvMove(m, level)) orderScore += 100000;
	return orderScore;
}