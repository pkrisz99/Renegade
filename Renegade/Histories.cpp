#include "Histories.h"

Histories::Histories() {
	ClearAll();
}

void Histories::ClearAll() {
	ClearKillerAndCounterMoves();
	std::memset(&QuietHistory, 0, sizeof(QuietHistory));
	std::memset(&CaptureHistory, 0, sizeof(CaptureHistory));
	std::memset(&ContinuationHistory, 0, sizeof(ContinuationHistory));
	std::memset(&MaterialCorrectionHistory, 0, sizeof(MaterialCorrectionHistory));
	std::memset(&PawnsCorrectionHistory, 0, sizeof(PawnsCorrectionHistory));
	std::memset(&FollowUpCorrectionHistory, 0, sizeof(FollowUpCorrectionHistory));
}

void Histories::ClearKillerAndCounterMoves() {
	std::memset(&KillerMoves, 0, sizeof(KillerMoves));
	std::memset(&CounterMoves, 0, sizeof(CounterMoves));
	std::memset(&PositionalMoves, 0, sizeof(PositionalMoves));
}

// Killer and countermoves ------------------------------------------------------------------------

void Histories::SetKillerMove(const Move& move, const int level) {
	if (level >= MaxDepth) return;
	KillerMoves[level] = move;
}

Move Histories::GetKillerMove(const int level) const {
	return KillerMoves[level];
}

void Histories::ResetKillerForPly(const int level) {
	KillerMoves[level] = NullMove;
}

void Histories::SetCountermove(const Move& previousMove, const Move& thisMove) {
	if (!previousMove.IsNull()) CounterMoves[previousMove.from][previousMove.to] = thisMove;
}

Move Histories::GetCountermove(const Move& previousMove) const {
	return CounterMoves[previousMove.from][previousMove.to];
}

void Histories::SetPositionalMove(const Position& pos, const Move& thisMove) {
	PositionalMoves[pos.Turn()][pos.GetPawnKey() % 8192] = thisMove;
}

Move Histories::GetPositionalMove(const Position& pos) const {
	return PositionalMoves[pos.Turn()][pos.GetPawnKey() % 8192];
}

// History heuristic ------------------------------------------------------------------------------

template void Histories::UpdateQuietHistory<Bonus>(const Position&, const Move&, int, int, int);
template void Histories::UpdateQuietHistory<Penalty>(const Position&, const Move&, int, int, int);
template void Histories::UpdateCaptureHistory<Bonus>(const Position&, const Move&, int, int);
template void Histories::UpdateCaptureHistory<Penalty>(const Position&, const Move&, int, int);

template <bool bonus>
void Histories::UpdateQuietHistory(const Position& position, const Move& m, const int level, const int depth, const int times) {
	
	const int delta = std::min(300 * depth, 2600) * times * (bonus ? 1 : -1);

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
	const int delta = std::min(300 * depth, 2600) * times * (bonus ? 1 : -1);
	const uint8_t attackingPiece = position.GetPieceAt(m.from);
	const uint8_t targetSquare = m.to;
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	const uint8_t capturedPiece = [&] {
		if (m.flag != MoveFlag::EnPassantPerformed) return position.GetPieceAt(m.to);
		else return (position.Turn() == Side::White) ? Piece::WhitePawn : Piece::BlackPawn;
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

int Histories::GetCaptureHistoryScore(const Position& position, const Move& m) const {
	const uint8_t attackingPiece = position.GetPieceAt(m.from);
	const uint8_t targetSquare = m.to;
	const bool fromSquareThreatened = position.IsSquareThreatened(m.from);
	const bool toSquareThreatened = position.IsSquareThreatened(m.to);
	const uint8_t capturedPiece = [&] {
		if (m.flag != MoveFlag::EnPassantPerformed) return position.GetPieceAt(m.to);
		else return (position.Turn() == Side::White) ? Piece::WhitePawn : Piece::BlackPawn;
	}();
	return CaptureHistory[attackingPiece][targetSquare][capturedPiece][fromSquareThreatened][toSquareThreatened];
}

// Static evaluation correction history -----------------------------------------------------------

void Histories::UpdateCorrection(const Position& position, const int16_t refEval, const int16_t score, const int depth) {
	const int diff = (score - refEval) * 256;
	const int weight = std::min(16, depth + 1);

	const uint64_t materialKey = position.GetMaterialKey() % 32768;
	int32_t& materialValue = MaterialCorrectionHistory[position.Turn()][materialKey];
	materialValue = ((226 - weight) * materialValue + weight * diff) / 226;
	materialValue = std::clamp(materialValue, -8350, 8350);

	const uint64_t pawnKey = position.GetPawnKey() % 16384;
	int32_t& pawnValue = PawnsCorrectionHistory[position.Turn()][pawnKey];
	pawnValue = ((226 - weight) * pawnValue + weight * diff) / 226;
	pawnValue = std::clamp(pawnValue, -8350, 8350);

	if (position.Moves.size() >= 2) {
		const MoveAndPiece& prev1 = position.GetPreviousMove(1);
		const MoveAndPiece& prev2 = position.GetPreviousMove(2);
		int32_t& followUpValue = FollowUpCorrectionHistory[prev2.piece][prev2.move.to][prev1.piece][prev1.move.to];
		followUpValue = ((226 - weight) * followUpValue + weight * diff) / 226;
		followUpValue = std::clamp(followUpValue, -8350, 8350);
	}
}

int16_t Histories::ApplyCorrection(const Position& position, const int16_t rawEval) const {
	if (std::abs(rawEval) >= MateThreshold) return rawEval;

	const uint64_t materialKey = position.GetMaterialKey() % 32768;
	const int materialCorrection = MaterialCorrectionHistory[position.Turn()][materialKey] / 256;

	const uint64_t pawnKey = position.GetPawnKey() % 16384;
	const int pawnCorrection = PawnsCorrectionHistory[position.Turn()][pawnKey] / 256;


	const int lastMoveCorrection = [&] {
		if (position.Moves.size() < 2) return 0;
		const MoveAndPiece& prev1 = position.GetPreviousMove(1);
		const MoveAndPiece& prev2 = position.GetPreviousMove(2);
		return FollowUpCorrectionHistory[prev2.piece][prev2.move.to][prev1.piece][prev1.move.to] / 256;
	}();

	const int correctedEval = rawEval + (materialCorrection + pawnCorrection + lastMoveCorrection);
	return std::clamp(correctedEval, -MateThreshold + 1, MateThreshold - 1);
}