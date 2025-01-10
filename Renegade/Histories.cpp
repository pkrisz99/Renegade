#include "Histories.h"

Histories::Histories() {
	ClearAll();
}

void Histories::ClearAll() {
	ClearKillerAndCounterMoves();
	std::memset(&QuietHistory, 0, sizeof(QuietHistoryTable));
	std::memset(&CaptureHistory, 0, sizeof(CaptureHistoryTable));
	std::memset(&ContinuationHistory, 0, sizeof(ContinuationHistoryTable));
	std::memset(&MaterialCorrectionHistory, 0, sizeof(MaterialCorrectionTable));
	std::memset(&PawnsCorrectionHistory, 0, sizeof(PawnsCorrectionTable));
	std::memset(&FollowUpCorrectionHistory, 0, sizeof(FollowUpCorrectionTable));
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

	// Main quiet history (note that we manually increase the delta here)
	const bool side = ColorOfPiece(piece) == PieceColor::White;
	const bool fromSquareAttacked = position.IsSquareThreatened(m.from);
	const bool toSquareAttacked = position.IsSquareThreatened(m.to);
	const int16_t quietHistoryDelta = delta + ((delta > 0) ? 300 : -300);
	UpdateHistoryValue(QuietHistory[piece][m.to][fromSquareAttacked][toSquareAttacked], quietHistoryDelta);

	// Continuation history
	for (const int ply : { 1, 2, 4 }) {
		if (level < ply) break;
		const auto& [prevMove, prevPiece] = position.GetPreviousMove(ply);
		if (prevPiece != Piece::None) {
			int16_t& value = ContinuationHistory[prevPiece][prevMove.to][piece][m.to];
			UpdateHistoryValue(value, delta);
		}
	}
}

void Histories::UpdateCaptureHistory(const Position& position, const Move& m, const int16_t delta) {
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

int16_t Histories::GetCaptureHistoryScore(const Position& position, const Move& m) const {
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

void Histories::UpdateCorrection(const Position& position, const int16_t rawEval, const int16_t score, const int depth) {
	const int diff = (score - rawEval) * 256;
	const int weight = std::min(16, depth + 1);

	const uint64_t materialKey = position.GetMaterialKey() % 32768;
	int32_t& materialValue = MaterialCorrectionHistory[position.Turn()][materialKey];
	materialValue = ((256 - weight) * materialValue + weight * diff) / 256;
	materialValue = std::clamp(materialValue, -6144, 6144);

	const uint64_t pawnKey = position.GetPawnKey() % 16384;
	int32_t& pawnValue = PawnsCorrectionHistory[position.Turn()][pawnKey];
	pawnValue = ((256 - weight) * pawnValue + weight * diff) / 256;
	pawnValue = std::clamp(pawnValue, -6144, 6144);

	if (position.Moves.size() >= 2) {
		const MoveAndPiece& prev1 = position.GetPreviousMove(1);
		const MoveAndPiece& prev2 = position.GetPreviousMove(2);
		int32_t& followUpValue = FollowUpCorrectionHistory[prev2.piece][prev2.move.to][prev1.piece][prev1.move.to];
		followUpValue = ((256 - weight) * followUpValue + weight * diff) / 256;
		followUpValue = std::clamp(followUpValue, -6144, 6144);
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