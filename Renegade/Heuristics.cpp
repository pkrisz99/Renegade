#include "Heuristics.h"

Heuristics::Heuristics() {
	HashedEntryCount = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
}

// Move ordering & clearing -----------------------------------------------------------------------

const int Heuristics::CalculateOrderScore(Board& board, const Move& m, const int level, const float phase, const bool onPv) {
	int orderScore = 0;
	const int attackingPiece = TypeOfPiece(board.GetPieceAt(m.from));
	const int attackedPiece = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 0 };
	if (attackedPiece != PieceType::None) {
		orderScore = values[attackedPiece] * 16 - values[attackingPiece] + 100000;
	}

	if (m.flag == MoveFlag::PromotionToQueen) orderScore += 950 + 200000;
	else if (m.flag == MoveFlag::PromotionToRook) orderScore += 500 + 200000;
	else if (m.flag == MoveFlag::PromotionToBishop) orderScore += 290 + 10000;
	else if (m.flag == MoveFlag::PromotionToKnight) orderScore += 310 + 10000;
	else if (m.flag == MoveFlag::EnPassantPerformed) orderScore += 100 + 100000;

	bool turn = board.Turn;
	if (turn == Turn::White) {
		orderScore -= LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, m.from)], Weights[IndexLatePSQT(attackingPiece, m.from)], phase);
		orderScore += LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, m.to)], Weights[IndexLatePSQT(attackingPiece, m.to)], phase);
	}
	else {
		orderScore -= LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, Mirror[m.from])], Weights[IndexLatePSQT(attackingPiece, Mirror[m.from])], phase);
		orderScore += LinearTaper(Weights[IndexEarlyPSQT(attackingPiece, Mirror[m.to])], Weights[IndexLatePSQT(attackingPiece, Mirror[m.to])], phase);
	}

	if (IsKillerMove(m, level)) orderScore += 10000;
	if (IsPvMove(m, level) && onPv) orderScore += 10000000;
	int historyScore = HistoryTables[turn][m.from][m.to];
	if ((board.GetPieceAt(m.to) == PieceType::None) && (m.flag == 0)) {
		//orderScore += std::min(200, historyScore / 128);
	}
	return orderScore;
}

void Heuristics::ClearEntries() {
	ClearTranspositionTable(); // Clear transposition table
	ClearKillerMoves(); // Reset killer moves
	ResetPvTable(); // Reset PV table
	ClearHistoryTable(); // Clear history heuristic data
}

// PV table ---------------------------------------------------------------------------------------

void Heuristics::UpdatePvTable(const Move& move, const int level, const bool leaf) {
	if (level < PvSize) PvTable[level][level] = move;
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

const bool Heuristics::IsPvMove(const Move& move, const int level) {
	if (level >= PvMoves.size()) return false;
	if (move == PvMoves[level]) return true;
	return false;
}

const std::vector<Move> Heuristics::GeneratePvLine() {
	std::vector<Move> list = std::vector<Move>();
	for (int i = 0; i < PvSize; i++) {
		Move m = PvTable[0][i];
		if (m.IsEmpty()) break;
		list.push_back(m);
	}
	return list;
}

void Heuristics::SetPvLine(const std::vector<Move> pv) {
	PvMoves = pv;
}

void Heuristics::ResetPvTable() {
	for (int i = 0; i < PvSize; i++) {
		for (int j = 0; j < PvSize; j++) PvTable[i][j] = Move();
	}
}

void Heuristics::ClearPvLine() {
	PvMoves.clear();
}

// Killer moves -----------------------------------------------------------------------------------

void Heuristics::AddKillerMove(const Move& move, const int level) {
	if (IsKillerMove(move, level)) return;
	if (level >= 32) return;
	KillerMoves[level][1] = KillerMoves[level][0];
	KillerMoves[level][0] = move;
}

const bool Heuristics::IsKillerMove(const Move& move, const int level) {
	if (KillerMoves[level][0] == move) return true;
	if (KillerMoves[level][1] == move) return true;
	return false;
}

void Heuristics::ClearKillerMoves() {
	KillerMoves.clear();
	KillerMoves.reserve(32);
	for (int i = 0; i < 100; i++) {
		std::array<Move, 2> a = std::array<Move, 2>();
		a[0] = Move(0, 0);
		a[1] = Move(0, 0);
		KillerMoves.push_back(a);
	}
}

// History heuristic ------------------------------------------------------------------------------

void Heuristics::AddCutoffHistory(const bool side, const int from, const int to, const int depth) {
	HistoryTables[side][from][to] += depth * depth;
}

void Heuristics::ClearHistoryTable() {
	for (int i = 0; i < HistoryTables[0].size(); i++) {
		std::fill(std::begin(HistoryTables[0][i]), std::end(HistoryTables[0][i]), 0);
		std::fill(std::begin(HistoryTables[1][i]), std::end(HistoryTables[1][i]), 0);
	}
}

// Transposition table ----------------------------------------------------------------------------

void Heuristics::AddTranspositionEntry(const uint64_t hash, const int score, const int scoreType) {
	if (EstimateTranspositionAllocations() >= MaximumHashMemory) return;
	HashedEntryCount += 1;
	TranspositionEntry entry;
	entry.score = score;
	entry.scoreType = scoreType;
	Hashes[hash] = entry;
}

const bool Heuristics::RetrieveTranspositionEntry(const uint64_t &hash, TranspositionEntry&entry) {
	if (Hashes.find(hash) != Hashes.end()) {
		TranspositionEntry found = Hashes[hash];
		entry = found;
		return true;
	}
	return false;
}

const int64_t Heuristics::EstimateTranspositionAllocations() {
	// https://stackoverflow.com/questions/25375202/how-to-measure-the-memory-usage-of-stdunordered-map
	// (data list + bucket index) * 1.5
	const size_t dataListSize = Hashes.size() * (sizeof(TranspositionEntry) + sizeof(void*));
	const size_t bucketIndexSize = Hashes.bucket_count() * (sizeof(void*) + sizeof(size_t));
	return static_cast<int>((dataListSize + bucketIndexSize) * 1.5);
}

void Heuristics::SetHashSize(const int megabytes) {
	MaximumHashMemory = megabytes * 1024ULL * 1024ULL;
}

const int Heuristics::GetHashfull() {
	if (MaximumHashMemory <= 0) return -1;
	int64_t hashfull = EstimateTranspositionAllocations() * 1000LL / MaximumHashMemory;
	return static_cast<int>(std::min(hashfull, 1000LL));
}

void Heuristics::ClearTranspositionTable() {
	Hashes.clear();
	HashedEntryCount = 0;
}

void Heuristics::ResetTranspositionAllocations() {
	// Swap trick
	std::unordered_map<uint64_t, TranspositionEntry> empty;
	std::swap(Hashes, empty);
}
