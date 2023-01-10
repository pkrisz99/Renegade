#pragma once
#include "Board.h"
#include "Move.h"

static const int MateEval = 1000000;
static const int NoEval = -666666666;
static const int NegativeInfinity = -333333333; // Inventing a new kind of math here
static const int PositiveInfinity = 444444444; // These numbers are easy to recognize if something goes wrong

const static int WeightsSize = 790;

// Source of values:
// https://www.chessprogramming.org/Simplified_Evaluation_Function

static const int Weights[WeightsSize] = {

	// 1. Pawn early PSQT
	  0,   0,   0,   0,   0,   0,   0,   0,
	  5,  10,  10, -20, -20,  10,  10,   5,
	  5,  -5,  -10,  0,   0,  -10,  5,   5,
	  0,   0,   0,  20,  20,  0,    0,    0,
	  5,   5,  10,  25,  25,  10,   5,   5,
	 10,  10,  20,  30,  30,  20,  10,  10,
	 50,  50,  50,  50,  50,  50,  50,  50,
	  0,   0,   0,   0,   0,   0,   0,   0,

	// 2. Knight early PSQT
	-50, -40, -30, -30, -30, -30, -40, -50,
	-40, -20,   0,   5,   5,   0, -20, -40,
	-30,   5,  10,  15,  15,  10,   5, -30,
	-30,   0,  15,  20,  20,  15,   0, -30,
	-30,   5,  15,  20,  20,  15,   5, -30,
	-30,   0,  10,  15,  15,  10,   0, -30,
	-40, -20,   0,   0,   0,   0, -20, -40,
	-50, -40, -30, -30, -30, -30, -40, -50,

	// 3. Bishop early PSQT
	-20, -10, -10, -10, -10, -10, -10, -20,
	-10,   5,   0,   0,   0,   0,   5, -10,
	-10,  10,  10,  10,  10,  10,  10, -10,
	-10,   0,  10,  10,  10,  10,   0, -10,
	-10,   5,   5,  10,  10,   5,   5, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-20, -10, -10, -10, -10, -10, -10, -20,

	// 4. Rook early PSQT
	 0,  0,  0,  5,  5,  0,  0,  0,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	 5, 10, 10, 10, 10, 10, 10,  5,
	 0,  0,  0,  0,  0,  0,  0,  0,

	 // 5. Queen early PSQT
	-20, -10, -10,  -5,  -5, -10, -10, -20,
	-10,   0,   5,   0,   0,   0,   0, -10,
	-10,   5,   5,   5,   5,   5,   0, -10,
	  0,   0,   5,   5,   5,   5,   0,  -5,
	 -5,   0,   5,   5,   5,   5,   0,  -5,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-20, -10, -10,  -5,  -5, -10, -10, -20,

	// 6. King early PSQT
	 20,  30,  10,   0,   0,  10,  30,  20,
	 20,  20,   0,   0,   0,   0,  20,  20,
	-10, -20, -20, -20, -20, -20, -20, -10,
	-20, -30, -30, -40, -40, -30, -30, -20,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,


	// 7. Pawn late PSQT
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	 10,  10,  15,  20,  20,  15,  10,  10,
	 20,  25,  30,  35,  35,  30,  25,  20,
	 40,  45,  50,  60,  60,  50,  45,  40,
	 70,  80,  80,  80,  80,  80,  80,  70,
	140, 150, 160, 160, 160, 160, 150, 140,
	  0,   0,   0,   0,   0,   0,   0,   0,

	// 8. Knight late PSQT
	-50, -40, -30, -30, -30, -30, -40, -50,
	-40, -20,   0,   5,   5,   0, -20, -40,
	-30,   5,  10,  15,  15,  10,   5, -30,
	-30,   0,  15,  20,  20,  15,   0, -30,
	-30,   5,  15,  20,  20,  15,   5, -30,
	-30,   0,  10,  15,  15,  10,   0, -30,
	-40, -20,   0,   0,   0,   0, -20, -40,
	-50, -40, -30, -30, -30, -30, -40, -50,

	// 9. Bishop late PSQT
	-20, -10, -10, -10, -10, -10, -10, -20,
	-10,   5,   0,   0,   0,   0,   5, -10,
	-10,  10,  10,  10,  10,  10,  10, -10,
	-10,   0,  10,  10,  10,  10,   0, -10,
	-10,   5,   5,  10,  10,   5,   5, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-20, -10, -10, -10, -10, -10, -10, -20,

	// 10. Rook late PSQT
	0,  0,  0,  5,  5,  0,  0,  0,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	5, 10, 10, 10, 10, 10, 10,  5,
	0,  0,  0,  0,  0,  0,  0,  0,

	// 11. Queen late PSQT
	-20, -10, -10,  -5,  -5, -10, -10, -20,
	-10,   0,   5,   0,   0,   0,   0, -10,
	-10,   5,   5,   5,   5,   5,   0, -10,
	  0,   0,   5,   5,   5,   5,   0,  -5,
	 -5,   0,   5,   5,   5,   5,   0,  -5,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-20, -10, -10,  -5,  -5, -10, -10, -20,

	// 12. King late PSQT
	-50, -30, -30, -30, -30, -30, -30, -50,
	-30, -30,   0,  0,  0,  0, -30, -30,
	-30, -10,  20, 30, 30, 20, -10, -30,
	-30, -10,  30, 40, 40, 30, -10, -30,
	-30, -10,  30, 40, 40, 30, -10, -30,
	-30, -10,  20, 30, 30, 20, -10, -30,
	-30, -20, -10,   0,   0, -10, -20, -30,
	-50, -40, -30, -20, -20, -30, -40, -50,

	// 13. Material early values
	100,
	305,
	310,
	500,
	950,
	0,

	// 14. Material late values
	100,
	280,
	330,
	500,
	950,
	0,

	// 15. Bishop pair bonus
	30,
	40,

	// 16. Tempo bonus
	10,
	0,

	// 17. Doubled and tripled pawn penalty
	-15,
	-15,
	-30,
	-30,

	// 18. Passed pawn bonus
	5,
	25,
};

class Evaluation
{
public:
	int score = 0;
	std::vector<Move> pv;
	int depth = 0;
	int seldepth = 0;
	int nodes = 0;
	int qnodes = 0;
	int time = 0;
	int nps = 0;
	int hashfull = 0;
	Evaluation();
	Evaluation(int s, std::vector<Move> p, int d, int sd, int n, int qn, int t, int speed, int h);
	Move BestMove();
};

static const int TaperedValue(const int earlyValue, const int lateValue, const float phase) {
	return (int)((1 - phase) * (float)earlyValue + phase * (float)lateValue);
}

static const float CalculateGamePhase(Board &board) {
	int remainingPieces = Popcount(board.GetOccupancy());
	float phase = (32.f - remainingPieces) / (32.f - 4.f);
	return std::clamp(phase, 0.f, 1.f);
}

// Weight indices:

//   0- 63: Pawn early PSQT
//  64-127: Knight early PSQT
// 128-191: Bishop early PSQT
// 192-255: Rook early PSQT
// 256-319: Queen early PSQT
// 320-383: King early PSQT

// 384-447: Pawn late PSQT
// 448-511: Knight late PSQT
// 512-575: Bishop late PSQT
// 576-639: Rook late PSQT
// 640-703: Queen late PSQT
// 704-767: King late PSQT

// 768-773: Material value early (includes king as well)
// 774-779: Material value early (includes king as well)
// 780-781: Bishop pair bonus


inline static constexpr int IndexEarlyPSQT(const int pieceType, const int location) {
	return (pieceType - 1) * 64 + location;
}

inline static constexpr int IndexLatePSQT(const int pieceType, const int location) {
	return (pieceType + 5) * 64 + location;
}

inline static constexpr int IndexPieceValueEarly(const int pieceType) {
	return 767 + pieceType;
}

inline static constexpr int IndexPieceValueLate(const int pieceType) {
	return 773 + pieceType;
}

const int IndexBishopPairEarly = 780;
const int IndexBishopPairLate = 781;
const int IndexTempoEarly = 782;
const int IndexTempoLate = 783;
const int IndexDoubledPawnEarly = 784;
const int IndexDoubledPawnLate = 785;
const int IndexTripledPawnEarly = 786;
const int IndexTripledPawnLate = 787;
const int IndexPassedPawnEarly = 788;
const int IndexPassedPawnLate = 789;



inline static const int EvaluateBoard(Board& board, const int level, const int weights[WeightsSize]) {

	// Game over?
	if (board.State == GameState::Draw) return 0;
	if (board.State == GameState::WhiteVictory) {
		if (board.Turn == Turn::White) return MateEval - (level + 1) / 2;
		if (board.Turn == Turn::Black) return -MateEval + (level + 1) / 2;
	}
	else if (board.State == GameState::BlackVictory) {
		if (board.Turn == Turn::White) return -MateEval + (level + 1) / 2;
		if (board.Turn == Turn::Black) return MateEval - (level + 1) / 2;
	}

	int score = 0;
	uint64_t occupancy = board.GetOccupancy();
	float phase = CalculateGamePhase(board);
	while (occupancy != 0) {
		int i = 63 - Lzcount(occupancy);
		SetBitFalse(occupancy, i);
		int piece = board.GetPieceAt(i);
		int pieceType = TypeOfPiece(piece);
		int pieceColor = ColorOfPiece(piece);

		if (pieceColor == PieceColor::White) {
			score += TaperedValue(weights[IndexEarlyPSQT(pieceType, i)], weights[IndexLatePSQT(pieceType, i)], phase);
			score += TaperedValue(weights[IndexPieceValueEarly(pieceType)], weights[IndexPieceValueLate(pieceType)], phase);
		}
		else if (pieceColor == PieceColor::Black) {
			score -= TaperedValue(weights[IndexEarlyPSQT(pieceType, Mirror[i])], weights[IndexLatePSQT(pieceType, Mirror[i])], phase);
			score -= TaperedValue(weights[IndexPieceValueEarly(pieceType)], weights[IndexPieceValueLate(pieceType)], phase);
		}

		// Passed pawn bonus
		if (piece == Piece::WhitePawn) {
			if (((WhitePassedPawnMask[i] & board.BlackPawnBits) == 0) && ((WhitePassedPawnFilter[i] & board.WhitePawnBits) == 0))
				score += TaperedValue(weights[IndexPassedPawnEarly], weights[IndexPassedPawnLate], phase);
		}
		else if (piece == Piece::BlackPawn) {
			if (((BlackPassedPawnMask[i] & board.WhitePawnBits) == 0) && ((BlackPassedPawnFilter[i] & board.BlackPawnBits) == 0))
				score -= TaperedValue(weights[IndexPassedPawnEarly], weights[IndexPassedPawnLate], phase);
		}
	}

	// Bishop pair bonus
	if (Popcount(board.WhiteBishopBits) >= 2) score += TaperedValue(weights[IndexBishopPairEarly], weights[IndexBishopPairLate], phase);
	if (Popcount(board.BlackBishopBits) >= 2) score -= TaperedValue(weights[IndexBishopPairEarly], weights[IndexBishopPairLate], phase);

	// Doubled pawn penalties
	for (int i = 0; i < 8; i++) {
		const int whitePawnsOnFile = Popcount(board.WhitePawnBits & Files[i]);
		const int blackPawnsOnFile = Popcount(board.BlackPawnBits & Files[i]);
		if (whitePawnsOnFile == 2) score += TaperedValue(weights[IndexDoubledPawnEarly], weights[IndexDoubledPawnLate], phase);
		if (whitePawnsOnFile > 2) score += TaperedValue(weights[IndexTripledPawnEarly], weights[IndexTripledPawnLate], phase);
		if (blackPawnsOnFile == 2) score -= TaperedValue(weights[IndexDoubledPawnEarly], weights[IndexDoubledPawnLate], phase);
		if (blackPawnsOnFile > 2) score -= TaperedValue(weights[IndexTripledPawnEarly], weights[IndexTripledPawnLate], phase);
	}

	if (!board.Turn) score *= -1;

	// Tempo bonus
	score -= TaperedValue(weights[IndexTempoEarly], weights[IndexTempoLate], phase);

	return score;
}

inline static const int EvaluateBoard(Board& board, const int level) {
	return EvaluateBoard(board, level, Weights);
}
