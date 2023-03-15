#include "Heuristics.h"

Heuristics::Heuristics() {
	TranspositionEntryCount = 0;
	KillerMoves.reserve(100);
	PvMoves = std::vector<Move>();
	SetHashSize(0);
}

// Move ordering & clearing -----------------------------------------------------------------------

const int Heuristics::CalculateOrderScore(Board& board, const Move& m, const int level, const float phase, const bool onPv, const Move& trMove) {
	int orderScore = 0;
	const int attackingPiece = TypeOfPiece(board.GetPieceAt(m.from));
	const int attackedPiece = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 0 };

	if (IsPvMove(m, level) && onPv) return 10000000; // ????
	if ((m.from == trMove.from) && (m.to == trMove.to) && (m.flag == trMove.flag)) return 9000000;

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
	int historyScore = HistoryTables[turn][m.from][m.to];
	if ((board.GetPieceAt(m.to) == PieceType::None) && (m.flag == 0)) {
		//orderScore += std::min(200, historyScore / 128);
	}
	return orderScore;
}

void Heuristics::ClearEntries() {
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

void Heuristics::AddTranspositionEntry(const uint64_t hash, const int depth, int score, const int scoreType, const Move bestMove, const int level) {
	if (HashBits == 0) return;
	uint64_t key = hash & HashFilter;
	if ((TranspositionTable[key].hash != hash) || (TranspositionTable[key].depth <= depth)) {
		if (TranspositionTable[key].hash == 0) TranspositionEntryCount += 1;
		TranspositionTable[key].hash = hash;
		TranspositionTable[key].depth = depth;
		TranspositionTable[key].moveFrom = bestMove.from;
		TranspositionTable[key].moveTo = bestMove.to;
		TranspositionTable[key].moveFlag = bestMove.flag;
		TranspositionTable[key].scoreType = scoreType;
		if (IsWinningMateScore(score)) score += level; // Adjusting mate scores with plies (some black magic stuff)
		if (IsLosingMateScore(score)) score -= level;
		TranspositionTable[key].score = score;
	}
}

const bool Heuristics::RetrieveTranspositionEntry(const uint64_t& hash, const int depth, TranspositionEntry& entry, const int level) {
	if (HashBits == 0) return false;
	uint64_t key = hash & HashFilter;
	if ((TranspositionTable[key].hash == hash)) {
		entry.hash = TranspositionTable[key].hash;
		entry.depth = TranspositionTable[key].depth;
		entry.moveFrom = TranspositionTable[key].moveFrom;
		entry.moveTo = TranspositionTable[key].moveTo;
		entry.moveFlag = TranspositionTable[key].moveFlag;
		entry.scoreType = TranspositionTable[key].scoreType;
		int score = TranspositionTable[key].score;
		if (IsLosingMateScore(score)) score += level;
		if (IsWinningMateScore(score)) score -= level;
		entry.score = score;
		return true;
	}
	return false;
}

void Heuristics::SetHashSize(const int megabytes) {
	if (megabytes == 0) {
		HashBits = 0;
		HashFilter = 0;
		TheoreticalTranspositionEntires = 0;
		ClearTranspositionTable();
		return;
	}

	TheoreticalTranspositionEntires = megabytes * 1024ULL * 1024ULL / sizeof(TranspositionEntry);
	int bits = 0;
	while ((1ULL << bits) <= TheoreticalTranspositionEntires) bits += 1;
	bits -= 1;
	HashFilter = (1ULL << bits) - 1;
	HashBits = bits;
	ClearTranspositionTable();
}

const int Heuristics::GetHashfull() {
	return static_cast<int>(TranspositionEntryCount * 1000LL / (HashFilter + 1LL));
}

void Heuristics::ClearTranspositionTable() {
	TranspositionTable.clear();
	TranspositionTable.reserve(HashFilter + 1);
	for (int i = 0; i < HashFilter; i++) TranspositionTable.push_back(TranspositionEntry());
	TranspositionTable.shrink_to_fit();
	TranspositionEntryCount = 0;
}

const void Heuristics::GetTranspositionInfo(uint64_t& trTheoretical, uint64_t& trUsable, uint64_t& trBits, uint64_t& trUsed) {
	trTheoretical = TheoreticalTranspositionEntires;
	trUsable = HashFilter + 1;
	trBits = HashBits;
	trUsed = TranspositionEntryCount;
}