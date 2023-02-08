#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
}

void Heuristics::AddEntry(const uint64_t hash, const int score, const int scoreType) {
	if (EstimateAllocatedMemory() >= MaximumHashMemory) return;
	HashedEntryCount += 1;
	HashEntry entry;
	entry.score = score;
	entry.scoreType = scoreType;
	Hashes[hash] = entry;
}

const std::tuple<bool, HashEntry> Heuristics::RetrieveEntry(const uint64_t &hash) {

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
	for (int i = 0; i < PvSize; i++) {
		for (int j = 0; j < PvSize; j++) PvTable[i][j] = Move();
	}
}

const int Heuristics::EstimateAllocatedMemory() {
	// https://stackoverflow.com/questions/25375202/how-to-measure-the-memory-usage-of-stdunordered-map
	// (data list + bucket index) * 1.5
	const size_t dataListSize = Hashes.size() * (sizeof(HashEntry) + sizeof(void*));
	const size_t bucketIndexSize = Hashes.bucket_count() * (sizeof(void*) + sizeof(size_t));
	return static_cast<int>((dataListSize + bucketIndexSize) * 1.5);
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
	if (KillerMoves[level][0] == move) return true;
	if (KillerMoves[level][1] == move) return true;
	return false;
}

const bool Heuristics::IsPvMove(const Move move, const int level) {
	if (level >= PvMoves.size()) return false;
	if (move == PvMoves[level]) return true;
	return false;
}

void Heuristics::SetHashSize(const int megabytes) {
	MaximumHashMemory = megabytes * 1024ULL * 1024ULL;
}

const int Heuristics::GetHashfull() {
	if (MaximumHashMemory <= 0) return -1;
	int64_t hashfull = EstimateAllocatedMemory() * 1000 / MaximumHashMemory;
	return static_cast<int>(std::min(hashfull, 1000LL));
}

void Heuristics::ResetHashStructure() {
	// Swap trick
	std::unordered_map<uint64_t, HashEntry> empty;
	std::swap(Hashes, empty);
}

void Heuristics::UpdatePvTable(const Move move, const int level, const bool leaf) {
	PvTable[level][level] = move;
	for (int i = level + 1; i < PvSize; i++) {
		Move lowerMove = PvTable[level + 1][i];
		if (lowerMove.IsEmpty()) break;
		PvTable[level][i] = lowerMove;
	}
	if (leaf) {
		for (int i = level + 1; i < PvSize; i++) {
			if (PvTable[level][i].IsEmpty()) break;
			PvTable[level][i] = Move();
		}
	}
	/*cout << "[" << endl;
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 6; j++) {
			cout << " " << PvTable[i][j].ToString();
		}
		cout << endl;
	}
	cout << "]" << endl;*/
}

const std::vector<Move> Heuristics::GetPvLine() {
	std::vector<Move> list = std::vector<Move>();
	for (int i = 0; i < PvSize; i++) {
		Move m = PvTable[0][i];
		if (m.IsEmpty()) break;
		list.push_back(m);
	}
	return list;
}

// Move ordering scoring function
const int Heuristics::CalculateOrderScore(Board board, const Move m, const int level, const float phase, const bool onPv) {
	int orderScore = 0;
	const int attackingPiece = TypeOfPiece(board.GetPieceAt(m.from));
	const int attackedPiece = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 0 };
	if (attackedPiece != PieceType::None) {
		orderScore = values[attackedPiece] - values[attackingPiece] + 10000;
	}

	if (m.flag == MoveFlag::PromotionToQueen) orderScore += 950 + 20000;
	else if (m.flag == MoveFlag::PromotionToRook) orderScore += 500 + 20000;
	else if (m.flag == MoveFlag::PromotionToBishop) orderScore += 290 + 1000;
	else if (m.flag == MoveFlag::PromotionToKnight) orderScore += 310 + 1000;
	else if (m.flag == MoveFlag::EnPassantPerformed) orderScore += 100 + 10000;

	bool turn = board.Turn;
	if (turn == Turn::White) {
		orderScore -= LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, m.from)], Weights[IndexLatePSQT(attackingPiece, m.from)], phase);
		orderScore += LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, m.to)], Weights[IndexLatePSQT(attackingPiece, m.to)], phase);
	}
	else {
		orderScore -= LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, Mirror[m.from])], Weights[IndexLatePSQT(attackingPiece, Mirror[m.from])], phase);
		orderScore += LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, Mirror[m.to])], Weights[IndexLatePSQT(attackingPiece, Mirror[m.to])], phase);
	}

	if (IsKillerMove(m, level)) orderScore += 1000;
	if (IsPvMove(m, level) && onPv) orderScore += 1000000;
	return orderScore;
}