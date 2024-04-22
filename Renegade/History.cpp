#include "History.h"

History::History() {
	ContinuationHistory = new Continuations;
	ClearHistory();
}

History::~History() {
	delete ContinuationHistory;
}

void History::ClearHistory() {
	std::memset(&HistoryTables, 0, sizeof(HistoryTables));
	std::memset(ContinuationHistory, 0, sizeof(Continuations));
}

void History::ClearKillerAndCounterMoves() {
	std::memset(&KillerMoves, 0, sizeof(KillerMoves));
	std::memset(&CounterMoves, 0, sizeof(CounterMoves));
}

// Killer and countermoves ------------------------------------------------------------------------

void History::AddKillerMove(const Move& move, const int level) {
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

bool History::IsKillerMove(const Move& move, const int level) const {
	if (KillerMoves[level][0] == move) return true;
	if (KillerMoves[level][1] == move) return true;
	return false;
}

bool History::IsFirstKillerMove(const Move& move, const int level) const {
	return KillerMoves[level][0] == move;
}

bool History::IsSecondKillerMove(const Move& move, const int level) const {
	return KillerMoves[level][1] == move;
}

void History::ResetKillersForPly(const int level) {
	KillerMoves[level][0] = EmptyMove;
	KillerMoves[level][1] = EmptyMove;
}

void History::AddCountermove(const Move& previousMove, const Move& thisMove) {
	if (!previousMove.IsNull()) CounterMoves[previousMove.from][previousMove.to] = thisMove;
}

bool History::IsCountermove(const Move& previousMove, const Move& thisMove) const {
	return CounterMoves[previousMove.from][previousMove.to] == thisMove;
}

// History heuristic ------------------------------------------------------------------------------

void History::UpdateHistory(const Move& m, const int16_t delta, const uint8_t piece, const int depth, const Position& position,
	const int level, const bool fromSquareAttacked, const bool toSquareAttacked) {

	// Main quiet history
	const bool side = ColorOfPiece(piece) == PieceColor::White;
	UpdateHistoryValue(HistoryTables[fromSquareAttacked][toSquareAttacked][piece][m.to], delta);

	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		const auto& [prevMove, prevPiece] = position.GetPreviousMove(ply);
		if (prevPiece != Piece::None) {
			int16_t& value = (*ContinuationHistory)[prevPiece][prevMove.to][piece][m.to];
			UpdateHistoryValue(value, delta);
		}
	}
}

int History::GetHistoryScore(const Position& position, const Move& m, const int level, const uint8_t movedPiece, const uint64_t opponentAttacks) const {
	int historyScore = HistoryTables[CheckBit(opponentAttacks, m.from)][CheckBit(opponentAttacks, m.to)][movedPiece][m.to];
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		historyScore += (*ContinuationHistory)[position.GetPreviousMove(ply).piece][position.GetPreviousMove(ply).move.to][movedPiece][m.to];
	}
	return historyScore;
}