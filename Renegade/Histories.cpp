#include "Histories.h"

Histories::Histories() {
	ContinuationHistory = std::make_unique<Continuations>();
	ClearAll();
}

void Histories::ClearAll() {
	ClearKillerAndCounterMoves();
	std::memset(&HistoryTables, 0, sizeof(HistoryTables));
	std::memset(ContinuationHistory.get(), 0, sizeof(Continuations));
}

void Histories::ClearKillerAndCounterMoves() {
	std::memset(&KillerMoves, 0, sizeof(KillerMoves));
	std::memset(&CounterMoves, 0, sizeof(CounterMoves));
}

// Killer and countermoves ------------------------------------------------------------------------

void Histories::AddKillerMove(const Move& move, const int level) {
	if (level >= MaxDepth) return;
	KillerMoves[level] = move;
}

bool Histories::IsKillerMove(const Move& move, const int level) const {
	return KillerMoves[level] == move;
}

void Histories::ResetKillerForPly(const int level) {
	KillerMoves[level] = EmptyMove;
}

void Histories::AddCountermove(const Move& previousMove, const Move& thisMove) {
	if (!previousMove.IsNull()) CounterMoves[previousMove.from][previousMove.to] = thisMove;
}

bool Histories::IsCountermove(const Move& previousMove, const Move& thisMove) const {
	return CounterMoves[previousMove.from][previousMove.to] == thisMove;
}

// History heuristic ------------------------------------------------------------------------------

void Histories::UpdateHistory(const Position& position, const Move& m, const uint8_t piece, const int16_t delta, const int level) {

	// Main quiet history
	const bool side = ColorOfPiece(piece) == PieceColor::White;
	const bool fromSquareAttacked = position.IsSquareThreatened(m.from);
	const bool toSquareAttacked = position.IsSquareThreatened(m.to);
	UpdateHistoryValue(HistoryTables[piece][m.to][fromSquareAttacked][toSquareAttacked], delta);

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

int Histories::GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const {
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	int historyScore = HistoryTables[movedPiece][m.to][fromSquareThreatened][toSquareThreatened];

	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		historyScore += (*ContinuationHistory)[position.GetPreviousMove(ply).piece][position.GetPreviousMove(ply).move.to][movedPiece][m.to];
	}
	return historyScore;
}