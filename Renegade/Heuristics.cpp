#include "Heuristics.h"

Heuristics::Heuristics() {
	ContinuationHistory = new Continuations;
	TranspositionEntryCount = 0;
	SetHashSize(1); // ???
	ClearHistoryTable();
}

Heuristics::~Heuristics() {
	delete ContinuationHistory;
}

// Move ordering & clearing -----------------------------------------------------------------------

int Heuristics::CalculateOrderScore(const Board& board, const Move& m, const int level, const float phase, const Move& ttMove,
	const std::array<MoveAndPiece, MaxDepth>& moveStack, const bool losingCapture, const bool useMoveStack, const uint64_t opponentAttacks) const {

	const uint8_t movedPiece = board.GetPieceAt(m.from);
	const int attackingPieceType = TypeOfPiece(movedPiece);
	const int capturedPieceType = TypeOfPiece(board.GetPieceAt(m.to));
	const int values[] = { 0, 100, 300, 300, 500, 900, 0 };
	
	// Transposition move
	if (m == ttMove) return 900000;

	// Queen promotions
	if (m.flag == MoveFlag::PromotionToQueen) return 700000 + values[capturedPieceType];

	// Captures
	if (!losingCapture) {
		if (capturedPieceType != PieceType::None) return 600000 + values[capturedPieceType] * 16 - values[attackingPieceType];
		if (m.flag == MoveFlag::EnPassantPerformed) return 600000 + values[PieceType::Pawn] * 16 - values[PieceType::Pawn];
	}
	else {
		if (capturedPieceType != PieceType::None) return -200000 + values[capturedPieceType] * 16 - values[attackingPieceType];
		if (m.flag == MoveFlag::EnPassantPerformed) return -200000 + values[PieceType::Pawn] * 16 - values[PieceType::Pawn];
	}
	
	// Quiet killer moves
	if (IsFirstKillerMove(m, level)) return 100100;
	if (IsSecondKillerMove(m, level)) return 100000;

	// Countermove heuristic
	if (level > 0 && useMoveStack && IsCountermove(moveStack[level - 1].move, m)) return 99000;

	// Quiet moves
	const bool turn = board.Turn;

	int historyScore = HistoryTables[CheckBit(opponentAttacks, m.from)][CheckBit(opponentAttacks, m.to)][movedPiece][m.to];
	if (level >= 1) historyScore += (*ContinuationHistory)[moveStack[level - 1].piece][moveStack[level - 1].move.to][movedPiece][m.to];
	if (level >= 2) historyScore += (*ContinuationHistory)[moveStack[level - 2].piece][moveStack[level - 2].move.to][movedPiece][m.to];
	if (level >= 4) historyScore += (*ContinuationHistory)[moveStack[level - 4].piece][moveStack[level - 4].move.to][movedPiece][m.to];

	if (historyScore != 0) {
		// When at least we have some history scores
		return historyScore;
	}

	// If we have don't have history scores, we order by PSQT delta
	// this is a very old leftover of Renegade, and probably has very little effect on strength
	int orderScore = 0;
	if (turn == Turn::White) {
		orderScore -= LinearTaper(Weights.GetPSQT(attackingPieceType, m.from).early, Weights.GetPSQT(attackingPieceType, m.from).late, phase);
		orderScore += LinearTaper(Weights.GetPSQT(attackingPieceType, m.to).early, Weights.GetPSQT(attackingPieceType, m.to).late, phase);
	}
	else {
		orderScore -= LinearTaper(Weights.GetPSQT(attackingPieceType, Mirror(m.from)).early, Weights.GetPSQT(attackingPieceType, Mirror(m.from)).late, phase);
		orderScore += LinearTaper(Weights.GetPSQT(attackingPieceType, Mirror(m.to)).early, Weights.GetPSQT(attackingPieceType, Mirror(m.to)).late, phase);
	}
	return orderScore; // orderScore / 4 did marginally better at fixed nodes

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
	if (previousMove.IsNotNull()) CounterMoves[previousMove.from][previousMove.to] = thisMove;
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

void Heuristics::IncrementHistory(const Move& m, const uint8_t piece, const int depth, const std::array<MoveAndPiece, MaxDepth>& moveStack, const int level,
	const bool fromSquareAttacked, const bool toSquareAttacked) {
	const int bonus = std::min(300 * (depth - 1), 2250);

	// Main quiet history
	const bool side = ColorOfPiece(piece) == PieceColor::White; 
	UpdateHistoryValue(HistoryTables[fromSquareAttacked][toSquareAttacked][piece][m.to], bonus);
	
	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		uint8_t prevPiece = moveStack[level - ply].piece;
		uint8_t prevTo = moveStack[level - ply].move.to;
		if (prevPiece != Piece::None) {
			int16_t& value = (*ContinuationHistory)[prevPiece][prevTo][piece][m.to];
			UpdateHistoryValue(value, bonus);
		}
	}

}

void Heuristics::DecrementHistory(const Move& m, const uint8_t piece, const int depth, const std::array<MoveAndPiece, MaxDepth>& moveStack, const int level,
	const bool fromSquareAttacked, const bool toSquareAttacked) {
	const int bonus = -1 * std::min(300 * (depth - 1), 2250);

	// Main quiet history
	const bool side = ColorOfPiece(piece) == PieceColor::White;
	UpdateHistoryValue(HistoryTables[fromSquareAttacked][toSquareAttacked][piece][m.to], bonus);

	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		uint8_t prevPiece = moveStack[level - ply].piece;
		uint8_t prevTo = moveStack[level - ply].move.to;
		if (prevPiece != Piece::None) {
			int16_t& value = (*ContinuationHistory)[prevPiece][prevTo][piece][m.to];
			UpdateHistoryValue(value, bonus);
		}
	}
}

inline void Heuristics::UpdateHistoryValue(int16_t& value, const int amount) {
	const int gravity = value * std::abs(amount) / 16384;
	value += amount - gravity;
}

void Heuristics::AgeHistory() {
	// Nah.
}

void Heuristics::ClearHistoryTable() {
	std::memset(&HistoryTables, 0, sizeof(HistoryTables));
	std::memset(ContinuationHistory, 0, sizeof(Continuations));
}

// Transposition table ----------------------------------------------------------------------------

void Heuristics::AddTranspositionEntry(const uint64_t hash, const uint16_t age, const int depth, int score, const int scoreType, const Move& bestMove, const int level) {

	assert(std::abs(score) > MateEval);
	assert(HashFilter != 0);
	if (std::abs(score) > MateEval) return;

	const uint64_t key = hash & HashFilter;
	const uint16_t quality = age * 2 + depth;
	const uint32_t storedHash = static_cast<uint32_t>((hash & 0xFFFFFFFF00000000) >> 32);

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
	assert(HashFilter != 0);
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
	assert(megabytes != 0);
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