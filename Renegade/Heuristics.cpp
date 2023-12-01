#include "Heuristics.h"

Heuristics::Heuristics() {
	TranspositionEntryCount = 0;
	SetHashSize(0);
	ClearHistoryTable();
}

// Move ordering & clearing -----------------------------------------------------------------------

int Heuristics::CalculateOrderScore(const Board& board, const Move& m, const int level, const float phase, const Move& ttMove,
	const Move& previousMove, const bool losingCapture) const {
	const int attackingPieceType = TypeOfPiece(board.GetPieceAt(m.from));
	const int capturedPieceType = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 0 };
	
	// PV and transposition moves
	if (m == ttMove) return 900000;

	// Captures
	if (!losingCapture) {
		if (capturedPieceType != PieceType::None) return 600000 + values[capturedPieceType] * 16 - values[attackingPieceType];
		if (m.flag == MoveFlag::EnPassantPerformed) return 600000 + values[PieceType::Pawn] * 16 - values[PieceType::Pawn];
	}
	else {
		if (capturedPieceType != PieceType::None) return -200000 + values[capturedPieceType] * 16 - values[attackingPieceType];
		if (m.flag == MoveFlag::EnPassantPerformed) return -200000 + values[PieceType::Pawn] * 16 - values[PieceType::Pawn];
	}

	// Queen promotions
	if (m.flag == MoveFlag::PromotionToQueen) return 700000 + values[capturedPieceType];
	
	// Quiet killer moves
	if (IsFirstKillerMove(m, level)) return 100100;
	if (IsSecondKillerMove(m, level)) return 100000;

	// Countermove heuristic
	if (IsCountermove(previousMove, m)) return 99000;

	// Quiet moves
	const bool turn = board.Turn;
	const int historyScore = HistoryTables[turn][m.from][m.to];
	if (historyScore != 0) {
		// When at least we have some history scores
		return historyScore > 0 ? (historyScore + 100) : (historyScore - 100);
	}
	else {
		int orderScore = 0;
		// Use PSQT change if not
		if (turn == Turn::White) {
			orderScore -= LinearTaper(Weights.GetPSQT(attackingPieceType, m.from).early, Weights.GetPSQT(attackingPieceType, m.from).late, phase);
			orderScore += LinearTaper(Weights.GetPSQT(attackingPieceType, m.to).early, Weights.GetPSQT(attackingPieceType, m.to).late, phase);
		}
		else {
			orderScore -= LinearTaper(Weights.GetPSQT(attackingPieceType, Mirror(m.from)).early, Weights.GetPSQT(attackingPieceType, Mirror(m.from)).late, phase);
			orderScore += LinearTaper(Weights.GetPSQT(attackingPieceType, Mirror(m.to)).early, Weights.GetPSQT(attackingPieceType, Mirror(m.to)).late, phase);
		}
		return orderScore;
	}
}

// PV table ---------------------------------------------------------------------------------------

void Heuristics::InitPvLength(const int level) {
	PvLength[level] = level;
}

void Heuristics::UpdatePvTable(const Move& move, const int level) {
	PvTable[level][level] = move;
	for (int nextLevel = level + 1; nextLevel < PvLength[level + 1]; nextLevel++) {
		PvTable[level][nextLevel] = PvTable[level + 1][nextLevel];
	}
	PvLength[level] = PvLength[level + 1];
}

void Heuristics::GeneratePvLine(std::vector<Move>& list) const {
	for (int i = 0; i < PvLength[0]; i++) {
		const Move& m = PvTable[0][i];
		if (m.IsEmpty()) break;
		list.push_back(m);
	}
}

void Heuristics::ResetPvTable() {
	for (int i = 0; i < MaxDepth; i++) {
		for (int j = 0; j < MaxDepth; j++) PvTable[i][j] = EmptyMove;
		PvLength[i] = 0; // ?
	}
}

// Killer moves -----------------------------------------------------------------------------------

void Heuristics::AddKillerMove(const Move& move, const int level) {
	if (level >= MaxDepth) return;
	if (IsFirstKillerMove(move, level)) return;
	if (IsSecondKillerMove(move, level)) {
		KillerMoves[level][1] = KillerMoves[level][0];
		KillerMoves[level][0] = move;
		return;
	}
	KillerMoves[level][1] = KillerMoves[level][0];
	KillerMoves[level][0] = move;
}

bool Heuristics::IsKillerMove(const Move& move, const int level) const {
	if (KillerMoves[level][0] == move) return true;
	if (KillerMoves[level][1] == move) return true;
	return false;
}

bool Heuristics::IsFirstKillerMove(const Move& move, const int level) const {
	if (KillerMoves[level][0] == move) return true;
	return false;
}

bool Heuristics::IsSecondKillerMove(const Move& move, const int level) const {
	if (KillerMoves[level][1] == move) return true;
	return false;
}

bool Heuristics::IsCountermove(const Move& previousMove, const Move& thisMove) const {
	return CounterMoves[previousMove.from][previousMove.to] == thisMove;
}

void Heuristics::AddCountermove(const Move& previousMove, const Move& thisMove) {
	CounterMoves[previousMove.from][previousMove.to] = thisMove;
}

void Heuristics::ClearKillerAndCounterMoves() {
	for (int i = 0; i < KillerMoves.size(); i++) {
		KillerMoves[i][0] = EmptyMove;
		KillerMoves[i][1] = EmptyMove;
	}
	for (int i = 0; i < CounterMoves.size(); i++) {
		std::fill(std::begin(CounterMoves[i]), std::end(CounterMoves[i]), EmptyMove);
	}
}

// History heuristic ------------------------------------------------------------------------------

void Heuristics::IncrementHistory(const bool side, const int from, const int to, const int depth) {
	const int bonus = std::min(depth * depth * 4, 400);
	const int gravity = HistoryTables[side][from][to] / 64;
	HistoryTables[side][from][to] += std::max(bonus - gravity, 0);
	if (HistoryTables[side][from][to] > 32767) {
		//cout << HistoryTables[side][from][to] << " " << endl;
		HistoryTables[side][from][to] = 32767;
	}
}

void Heuristics::DecrementHistory(const bool side, const int from, const int to, const int depth) {
	const int bonus = -1 * std::min(depth * depth * 2, 200);
	const int gravity = HistoryTables[side][from][to] / 64;
	HistoryTables[side][from][to] += std::min(bonus - gravity, 0);
	if (HistoryTables[side][from][to] < -32768) {
		//cout << HistoryTables[side][from][to] << " " << endl;
		HistoryTables[side][from][to] = -32768;
	}
}

void Heuristics::AgeHistory() {
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			HistoryTables[0][i][j] /= 2;
			HistoryTables[1][i][j] /= 2;
		}
	}
}

void Heuristics::ClearHistoryTable() {
	for (int i = 0; i < HistoryTables[0].size(); i++) {
		std::fill(std::begin(HistoryTables[0][i]), std::end(HistoryTables[0][i]), 0);
		std::fill(std::begin(HistoryTables[1][i]), std::end(HistoryTables[1][i]), 0);
	}
}

// Transposition table ----------------------------------------------------------------------------

void Heuristics::AddTranspositionEntry(const uint64_t hash, const uint16_t age, const int depth, int score, const int scoreType, const Move& bestMove, const int level) {
	if (HashBits == 0) return;
	const uint64_t key = hash & HashFilter;
	const uint16_t quality = age * 2 + depth;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);

	if (std::abs(score) > MateEval) return;

	if (quality >= TranspositionTable[key].quality) { // (TranspositionTable[key].depth <= depth)
		if (TranspositionTable[key].hash == 0) TranspositionEntryCount += 1;
		TranspositionTable[key].depth = depth;
		if ((TranspositionTable[key].hash != storedHash) || (bestMove.IsNotNull())) {
			TranspositionTable[key].moveFrom = bestMove.from;
			TranspositionTable[key].moveTo = bestMove.to;
			TranspositionTable[key].moveFlag = bestMove.flag;
		}
		TranspositionTable[key].scoreType = scoreType;
		TranspositionTable[key].hash = storedHash;
		TranspositionTable[key].quality = quality;
		if (IsWinningMateScore(score)) score += level; // Adjusting mate scores with plies (some black magic stuff)
		else if (IsLosingMateScore(score)) score -= level;
		TranspositionTable[key].score = score;
	}
}

bool Heuristics::RetrieveTranspositionEntry(const uint64_t& hash, TranspositionEntry& entry, const int level) {
	if (HashBits == 0) return false;
	const uint64_t key = hash & HashFilter;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);
	if (TranspositionTable[key].hash == storedHash) {
		entry.depth = TranspositionTable[key].depth;
		entry.moveFrom = TranspositionTable[key].moveFrom;
		entry.moveTo = TranspositionTable[key].moveTo;
		entry.moveFlag = TranspositionTable[key].moveFlag;
		entry.scoreType = TranspositionTable[key].scoreType;
		entry.quality = TranspositionTable[key].quality; // not needed
		entry.hash = storedHash;
		int score = TranspositionTable[key].score;
		if (IsLosingMateScore(score)) score += level;
		else if (IsWinningMateScore(score)) score -= level;
		entry.score = score;
		return true;
	}
	return false;
}


void Heuristics::PrefetchTranspositionEntry(const uint64_t hash) const {
#if defined(__clang__)
	const uint64_t key = hash & HashFilter;
	__builtin_prefetch(&TranspositionTable[key]);
#endif
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

int Heuristics::GetHashfull() {
	return static_cast<int>(TranspositionEntryCount * 1000LL / (HashFilter + 1LL));
}

void Heuristics::ClearTranspositionTable() {
	TranspositionTable.clear();
	TranspositionTable.reserve(HashFilter + 1);
	for (uint64_t i = 0; i < HashFilter + 1; i++) TranspositionTable.push_back(TranspositionEntry());
	TranspositionTable.shrink_to_fit();
	TranspositionEntryCount = 0;
}

void Heuristics::GetTranspositionInfo(uint64_t& ttTheoretical, uint64_t& ttUsable, uint64_t& ttBits, uint64_t& ttUsed) {
	ttTheoretical = TheoreticalTranspositionEntires;
	ttUsable = HashFilter + 1;
	ttBits = HashBits;
	ttUsed = TranspositionEntryCount;
}

bool TranspositionEntry::IsCutoffPermitted(const int searchDepth, const int alpha, const int beta) const {
	if (searchDepth > depth) return false;

	return (scoreType == ScoreType::Exact)
		|| ((scoreType == ScoreType::UpperBound) && (score <= alpha))
		|| ((scoreType == ScoreType::LowerBound) && (score >= beta));
}