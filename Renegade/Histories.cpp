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
	std::memset(&PawnCorrectionHistoryUpperBits, 0, sizeof(PawnCorrectionHistoryUpperBits));
	std::memset(&NonPawnCorrectionHistory, 0, sizeof(NonPawnCorrectionHistory));
	std::memset(&NonPawnCorrectionHistoryUpperBits, 0, sizeof(NonPawnCorrectionHistoryUpperBits));
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
	
	const int deltaMain = bonus ? std::min(308 * depth, 3350) * times : -std::min(330 * depth, 3350);

	// Main quiet history
	const uint8_t movedPiece = position.GetPieceAt(m.from);
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	UpdateHistoryValue(QuietHistory[movedPiece][m.to][fromSquareThreatened][toSquareThreatened], deltaMain, 14500);

	// Get continuation history total
	int contHistTotal = 0;
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		contHistTotal += ContinuationHistory[position.GetPreviousMove(ply).piece][position.GetPreviousMove(ply).move.to][movedPiece][m.to];
	}

	const int deltaContHist = bonus ? std::min(292 * depth, 3200) * times : -std::min(316 * depth, 3200);

	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		const auto& [prevMove, prevPiece] = position.GetPreviousMove(ply);
		if (prevPiece != Piece::None) {
			int16_t& value = ContinuationHistory[prevPiece][prevMove.to][movedPiece][m.to];
			UpdateHistoryValueCustomGravity(value, contHistTotal, deltaContHist, 16230);
		}
	}
}

template <bool bonus>
void Histories::UpdateCaptureHistory(const Position& position, const Move& m, const int depth, const int times) {
	const int delta = bonus ? std::min(303 * depth, 2950) * times : -std::min(274 * depth, 2950);
	const uint8_t attackingPiece = position.GetPieceAt(m.from);
	const uint8_t targetSquare = m.to;
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	const uint8_t capturedPiece = [&] {
		if (m.flag != MoveFlag::EnPassantPerformed) return position.GetPieceAt(m.to);
		else return (position.Turn() == Side::White) ? Piece::BlackPawn : Piece::WhitePawn;
	}();
	UpdateHistoryValue(CaptureHistory[attackingPiece][targetSquare][capturedPiece][fromSquareThreatened][toSquareThreatened], delta, 18600);
}

int Histories::GetQuietHistoryScore(const Position& position, const Move& m, const uint8_t movedPiece, const int level) const {
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	int historyScore = QuietHistory[movedPiece][m.to][fromSquareThreatened][toSquareThreatened];

	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		historyScore += ContinuationHistory[position.GetPreviousMove(ply).piece][position.GetPreviousMove(ply).move.to][movedPiece][m.to];
	}
	return historyScore;
}

int Histories::GetCaptureHistoryScore(const Position& position, const Move& m) const {
	const uint8_t attackingPiece = position.GetPieceAt(m.from);
	const uint8_t targetSquare = m.to;
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	const uint8_t capturedPiece = [&] {
		if (m.flag != MoveFlag::EnPassantPerformed) return position.GetPieceAt(m.to);
		else return (position.Turn() == Side::White) ? Piece::BlackPawn : Piece::WhitePawn;
	}();
	return CaptureHistory[attackingPiece][targetSquare][capturedPiece][fromSquareThreatened][toSquareThreatened];
}

// Static evaluation correction history -----------------------------------------------------------

void Histories::UpdateCorrection(const Position& position, const int16_t refEval, const int16_t score, const int depth) {
	const int inertia = 151;
	const int cap = 12000;
	const int diff = (score - refEval) * 256;
	const int weight = std::min(16, depth + 1);

	// Pawn structure correction history update:

	const uint64_t pawnHash = position.GetPawnHash();
	const uint64_t pawnKey = pawnHash % 16384;
	const uint16_t newPawnUpperBits = pawnHash >> 48;
	uint16_t& oldPawnUpperBits = PawnCorrectionHistoryUpperBits[position.Turn()][pawnKey];
	int32_t& pawnValue = PawnCorrectionHistory[position.Turn()][pawnKey];

	if (oldPawnUpperBits != newPawnUpperBits) pawnValue = 0;
	pawnValue = ((inertia - weight) * pawnValue + weight * diff) / inertia;
	pawnValue = std::clamp(pawnValue, -cap, cap);
	oldPawnUpperBits = newPawnUpperBits;

	// Non-pawn structure correction history update:

	const auto [whiteNonPawnHash, blackNonPawnHash] = position.GetNonPawnHashes();
	const uint64_t whiteNonPawnKey = whiteNonPawnHash % 65536, blackNonPawnKey = blackNonPawnHash % 65536;
	const uint16_t newWhiteNonPawnUpperBits = whiteNonPawnHash >> 48, newBlackNonPawnUpperBits = blackNonPawnHash >> 48;

	int32_t& whiteNonPawnValue = NonPawnCorrectionHistory[position.Turn()][Side::White][whiteNonPawnKey];
	uint16_t& oldWhiteNonPawnUpperBits = NonPawnCorrectionHistoryUpperBits[position.Turn()][Side::White][whiteNonPawnKey];
	if (oldWhiteNonPawnUpperBits != newWhiteNonPawnUpperBits) whiteNonPawnValue = 0;
	whiteNonPawnValue = ((inertia - weight) * whiteNonPawnValue + weight * diff) / inertia;
	whiteNonPawnValue = std::clamp(whiteNonPawnValue, -cap, cap);
	oldWhiteNonPawnUpperBits = newWhiteNonPawnUpperBits;

	int32_t& blackNonPawnValue = NonPawnCorrectionHistory[position.Turn()][Side::Black][blackNonPawnKey];
	uint16_t& oldBlackNonPawnUpperBits = NonPawnCorrectionHistoryUpperBits[position.Turn()][Side::Black][blackNonPawnKey];
	if (oldBlackNonPawnUpperBits != newBlackNonPawnUpperBits) blackNonPawnValue = 0;
	blackNonPawnValue = ((inertia - weight) * blackNonPawnValue + weight * diff) / inertia;
	blackNonPawnValue = std::clamp(blackNonPawnValue, -cap, cap);
	oldBlackNonPawnUpperBits = newBlackNonPawnUpperBits;

	// Continuation correction history update:

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

	// Pawn structure correction history retrieval:

	const uint64_t pawnHash = position.GetPawnHash();
	const uint64_t pawnKey = pawnHash % 16384;
	const uint16_t pawnUpperBits = pawnHash >> 48;

	const int pawnCorrection = [&] {
		if (pawnUpperBits != PawnCorrectionHistoryUpperBits[position.Turn()][pawnKey]) return 0;
		return PawnCorrectionHistory[position.Turn()][pawnKey];
	}();

	// Non-pawn structure correction history retrieval:

	const auto [whiteNonPawnHash, blackNonPawnHash] = position.GetNonPawnHashes();
	const uint64_t whiteNonPawnKey = whiteNonPawnHash % 65536, blackNonPawnKey = blackNonPawnHash % 65536;
	const uint16_t whiteNonPawnUpperBits = whiteNonPawnHash >> 48, blackNonPawnUpperBits = blackNonPawnHash >> 48;

	const int whiteNonPawnCorrection = [&] {
		if (whiteNonPawnUpperBits != NonPawnCorrectionHistoryUpperBits[position.Turn()][Side::White][whiteNonPawnKey]) return 0;
		return NonPawnCorrectionHistory[position.Turn()][Side::White][whiteNonPawnKey];
	}();
	const int blackNonPawnCorrection = [&] {
		if (blackNonPawnUpperBits != NonPawnCorrectionHistoryUpperBits[position.Turn()][Side::Black][blackNonPawnKey]) return 0;
		return NonPawnCorrectionHistory[position.Turn()][Side::Black][blackNonPawnKey];
	}();

	const int nonPawnCorrection = whiteNonPawnCorrection + blackNonPawnCorrection;

	// Continuation correction history (contcorr) retrieval:

	const int lastMoveCorrection = [&] {
		if (position.Moves.size() < 2) return 0;
		const MoveAndPiece& prev1 = position.GetPreviousMove(1);
		const MoveAndPiece& prev2 = position.GetPreviousMove(2);
		return FollowUpCorrectionHistory[prev2.piece][prev2.move.to][prev1.piece][prev1.move.to];
	}();

	const int correctedEval = rawEval + (pawnCorrection * 231 + lastMoveCorrection * 243 + nonPawnCorrection * 256) / 256 / 256;
	return std::clamp(correctedEval, -MateThreshold + 1, MateThreshold - 1);
}
