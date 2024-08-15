#include "Histories.h"

Histories::Histories() {
	Continuations = std::make_unique<ContinuationTable>();
	ClearAll();
}

void Histories::ClearAll() {
	ClearKillerAndCounterMoves();
	std::memset(&QuietHistory, 0, sizeof(ThreatHistoryTable));
	std::memset(Continuations.get(), 0, sizeof(ContinuationTable));
}

void Histories::ClearKillerAndCounterMoves() {
	std::memset(&KillerMoves, 0, sizeof(KillerMoves));
	std::memset(&CounterMoves, 0, sizeof(CounterMoves));
}

// Killer and countermoves ------------------------------------------------------------------------

void Histories::SetKillerMove(const Move& move, const int level) {
	if (level >= MaxDepth) return;
	KillerMoves[level] = move;
}

bool Histories::IsKillerMove(const Move& move, const int level) const {
	return KillerMoves[level] == move;
}

void Histories::ResetKillerForPly(const int level) {
	KillerMoves[level] = EmptyMove;
}

void Histories::SetCountermove(const Move& previousMove, const Move& thisMove) {
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
	UpdateHistoryValue(QuietHistory[piece][m.to][fromSquareAttacked][toSquareAttacked], delta);

	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		const auto& [prevMove, prevPiece] = position.GetPreviousMove(ply);
		if (prevPiece != Piece::None) {
			int16_t& value = (*Continuations)[prevPiece][prevMove.to][piece][m.to];
			UpdateHistoryValue(value, delta);
		}
	}
}

int Histories::GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const {
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	int historyScore = QuietHistory[movedPiece][m.to][fromSquareThreatened][toSquareThreatened];

	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		historyScore += (*Continuations)[position.GetPreviousMove(ply).piece][position.GetPreviousMove(ply).move.to][movedPiece][m.to];
	}
	return historyScore;
}