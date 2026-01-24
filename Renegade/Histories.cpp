#include "Histories.h"

Histories::Histories() {
	ClearAll();
}

void Histories::ClearAll() {
	ClearRefutations();
	std::memset(&QuietHistory, 0, sizeof(QuietHistory));
	std::memset(&CaptureHistory, 0, sizeof(CaptureHistory));
	std::memset(&ContinuationHistory, 0, sizeof(ContinuationHistory));
	std::memset(&PawnCorrectionHistory, 0, sizeof(PawnCorrectionHistory));
	std::memset(&NonPawnCorrectionHistory, 0, sizeof(NonPawnCorrectionHistory));
	std::memset(&FollowUpCorrectionHistory, 0, sizeof(FollowUpCorrectionHistory));
}

void Histories::ClearRefutations() {
	std::memset(&KillerMoves, 0, sizeof(KillerMoves));
	std::memset(&CounterMoves, 0, sizeof(CounterMoves));
	std::memset(&PositionalMoves, 0, sizeof(PositionalMoves));
}

// Refutation moves -------------------------------------------------------------------------------
// Note that Renegade has 3 types of this, rather than the usual 2 (or even 1)
// Positional refutations are the ones that depend on the pawn structure

void Histories::SetKillerMove(const Move& move, const int level) {
	if (level >= MaxDepth) return;
	KillerMoves[level] = move;
}

void Histories::SetCountermove(const Move& previousMove, const Move& thisMove) {
	if (!previousMove.IsNull()) CounterMoves[previousMove.from][previousMove.to] = thisMove;
}

void Histories::SetPositionalMove(const Position& pos, const Move& thisMove) {
	PositionalMoves[pos.Turn()][pos.GetPawnHash() % 8192] = thisMove;
}

std::tuple<Move, Move, Move> Histories::GetRefutationMoves(const Position& pos, const int level) const {
	const Move previous = (level > 0) ? pos.GetPreviousMove(1).move : NullMove;
	const Move killer = KillerMoves[level];
	const Move counter = CounterMoves[previous.from][previous.to];
	const Move positional = PositionalMoves[pos.Turn()][pos.GetPawnHash() % 8192];
	return { killer, counter, positional };
}

void Histories::ResetKillerForPly(const int level) {
	KillerMoves[level] = NullMove;
}

// History heuristic ------------------------------------------------------------------------------

template void Histories::UpdateQuietHistory<Bonus>(const Position&, const Move&, int, int, int);
template void Histories::UpdateQuietHistory<Penalty>(const Position&, const Move&, int, int, int);
template void Histories::UpdateCaptureHistory<Bonus>(const Position&, const Move&, int, int);
template void Histories::UpdateCaptureHistory<Penalty>(const Position&, const Move&, int, int);

template <bool bonus>
void Histories::UpdateQuietHistory(const Position& position, const Move& m, const int level, const int depth, const int times) {
	
	const int delta = std::min(302 * depth, 3160) * times * (bonus ? 1 : -1);

	// Main quiet history
	const uint8_t movedPiece = position.GetPieceAt(m.from);
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	UpdateHistoryValue(QuietHistory[movedPiece][m.to][fromSquareThreatened][toSquareThreatened], delta);

	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		const auto& [prevMove, prevPiece] = position.GetPreviousMove(ply);
		if (prevPiece != Piece::None) {
			int16_t& value = ContinuationHistory[prevPiece][prevMove.to][movedPiece][m.to];
			UpdateHistoryValue(value, delta);
		}
	}
}

template <bool bonus>
void Histories::UpdateCaptureHistory(const Position& position, const Move& m, const int depth, const int times) {
	const int delta = std::min(302 * depth, 3160) * times * (bonus ? 1 : -1);
	const uint8_t attackingPiece = position.GetPieceAt(m.from);
	const uint8_t targetSquare = m.to;
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	const uint8_t capturedPiece = [&] {
		if (m.flag != MoveFlag::EnPassantPerformed) return position.GetPieceAt(m.to);
		else return (position.Turn() == Side::White) ? Piece::BlackPawn : Piece::WhitePawn;
	}();
	UpdateHistoryValue(CaptureHistory[attackingPiece][targetSquare][capturedPiece][fromSquareThreatened][toSquareThreatened], delta);
}

int Histories::GetHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const {
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	int historyScore = QuietHistory[movedPiece][m.to][fromSquareThreatened][toSquareThreatened];

	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		historyScore += ContinuationHistory[position.GetPreviousMove(ply).piece][position.GetPreviousMove(ply).move.to][movedPiece][m.to];
	}
	return historyScore;
}

int Histories::GetCaptureHistoryScore(const Board& board, const Move& m) const {
	const uint8_t attackingPiece = board.GetPieceAt(m.from);
	const uint8_t targetSquare = m.to;
	const bool fromSquareThreatened = board.IsSquareThreatened(m.from);
	const bool toSquareThreatened = board.IsSquareThreatened(m.to);
	const uint8_t capturedPiece = [&] {
		if (m.flag != MoveFlag::EnPassantPerformed) return board.GetPieceAt(m.to);
		else return (board.Turn == Side::White) ? Piece::BlackPawn : Piece::WhitePawn;
	}();
	return CaptureHistory[attackingPiece][targetSquare][capturedPiece][fromSquareThreatened][toSquareThreatened];
}

// Static evaluation correction history -----------------------------------------------------------

void Histories::UpdateCorrection(const Position& position, const int16_t refEval, const int16_t score, const int depth) {
	static constexpr int inertia = 143;
	static constexpr int cap = 9710;
	const int diff = (score - refEval) * 256;
	const int weight = std::min(16, depth + 1);

	const uint64_t pawnKey = position.GetPawnHash() % 16384;
	int32_t& pawnValue = PawnCorrectionHistory[position.Turn()][pawnKey];
	pawnValue = ((inertia - weight) * pawnValue + weight * diff) / inertia;
	pawnValue = std::clamp(pawnValue, -cap, cap);

	const auto [whiteNonPawnHash, blackNonPawnHash] = position.GetNonPawnHashes();
	const uint64_t whiteNonPawnKey = whiteNonPawnHash % 65536, blackNonPawnKey = blackNonPawnHash % 65536;
	int32_t& whiteNonPawnValue = NonPawnCorrectionHistory[position.Turn()][Side::White][whiteNonPawnKey];
	int32_t& blackNonPawnValue = NonPawnCorrectionHistory[position.Turn()][Side::Black][blackNonPawnKey];
	whiteNonPawnValue = ((inertia - weight) * whiteNonPawnValue + weight * diff) / inertia;
	blackNonPawnValue = ((inertia - weight) * blackNonPawnValue + weight * diff) / inertia;
	whiteNonPawnValue = std::clamp(whiteNonPawnValue, -cap, cap);
	blackNonPawnValue = std::clamp(blackNonPawnValue, -cap, cap);

	if (position.Moves.size() >= 2) {
		const MoveAndPiece& prev1 = position.GetPreviousMove(1);
		const MoveAndPiece& prev2 = position.GetPreviousMove(2);
		int32_t& followUpValue = FollowUpCorrectionHistory[prev2.piece][prev2.move.to][prev1.piece][prev1.move.to];
		followUpValue = ((inertia - weight) * followUpValue + weight * diff) / inertia;
		followUpValue = std::clamp(followUpValue, -cap, cap);
	}
}

int16_t Histories::ApplyCorrection(const Position& position, const int16_t rawEval) const {
	if (std::abs(rawEval) >= MateThreshold) return rawEval;

	const uint64_t pawnKey = position.GetPawnHash() % 16384;
	const int pawnCorrection = PawnCorrectionHistory[position.Turn()][pawnKey];

	const auto [whiteNonPawnHash, blackNonPawnHash] = position.GetNonPawnHashes();
	const uint64_t whiteNonPawnKey = whiteNonPawnHash % 65536, blackNonPawnKey = blackNonPawnHash % 65536;
	const int nonPawnCorrection = NonPawnCorrectionHistory[position.Turn()][Side::White][whiteNonPawnKey]
		+ NonPawnCorrectionHistory[position.Turn()][Side::Black][blackNonPawnKey];

	const int lastMoveCorrection = [&] {
		if (position.Moves.size() < 2) return 0;
		const MoveAndPiece& prev1 = position.GetPreviousMove(1);
		const MoveAndPiece& prev2 = position.GetPreviousMove(2);
		return FollowUpCorrectionHistory[prev2.piece][prev2.move.to][prev1.piece][prev1.move.to];
	}();

	const int correctedEval = rawEval + (pawnCorrection + lastMoveCorrection + nonPawnCorrection) / 256;
	return std::clamp(correctedEval, -MateThreshold + 1, MateThreshold - 1);
}
