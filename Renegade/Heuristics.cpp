#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	ApproxHashSize = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
}

void Heuristics::AddEntry(uint64_t hash, int score, int scoreType) {
	if (ApproxHashSize + sizeof(HashEntry) >= MaximumHashSize) return;
	HashedEntryCount += 1;
	HashEntry entry;
	entry.score = score;
	entry.scoreType = scoreType;
	Hashes[hash] = entry;
	ApproxHashSize += sizeof(HashEntry); // Is this good?
}

std::tuple<bool, HashEntry> Heuristics::RetrieveEntry(uint64_t hash) {

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

int Heuristics::CalculateMoveOrderScore(Board board, Move m, int level) {
	int orderScore = 0;
	int attackingPiece = TypeOfPiece(board.GetPieceAt(m.from));
	int attackedPiece = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 10000 };
	if (attackedPiece != PieceType::None) {
		orderScore = values[attackedPiece] - values[attackingPiece] + 10;
	}

	if (m.flag == MoveFlag::PromotionToQueen) orderScore += 900;
	else if (m.flag == MoveFlag::PromotionToRook) orderScore += 500;
	else if (m.flag == MoveFlag::PromotionToBishop) orderScore += 300;
	else if (m.flag == MoveFlag::PromotionToKnight) orderScore += 300;

	if (attackingPiece == PieceType::Pawn) orderScore += PawnPSQT[m.to] - PawnPSQT[m.from];
	else if (attackingPiece == PieceType::Knight) orderScore += KnightPSQT[m.to] - KnightPSQT[m.from];
	else if (attackingPiece == PieceType::Bishop) orderScore += BishopPSQT[m.to] - BishopPSQT[m.from];
	else if (attackingPiece == PieceType::Rook) orderScore += RookPSQT[m.to] - RookPSQT[m.from];
	else if (attackingPiece == PieceType::Queen) orderScore += QueenPSQT[m.to] - QueenPSQT[m.from];

	if (IsKillerMove(m, level)) orderScore += 200000;
	if (IsPvMove(m, level)) orderScore += 100000;
}