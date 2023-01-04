#pragma once
#include "Move.h"
#include "Board.h"

static const int MateEval = 1000000;
static const int NoEval = -666666666;
static const int NegativeInfinity = -333333333; // Inventing a new kind of math here
static const int PositiveInfinity = 444444444; // These numbers are easy to recognize if something goes wrong

const static int WeightsSize = 781;

// Source of values:
// https://www.chessprogramming.org/Simplified_Evaluation_Function

/*
const class Weights {
public:

	static const int PawnValue = 100;
	static const int KnightValue = 300;
	static const int BishopValue = 300;
	static const int RookValue = 500;
	static const int QueenValue = 900;
	static constexpr int PieceMaterial[] = { 0, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, 0 };

	static const int BishopPairBonus = 50;

	static constexpr int PawnPSQT[64] = {
		 0,   0,   0,   0,   0,   0,   0,   0,
		 0,   0,   0,   0,   0,   0,   0,   0,
		10,  10,  15,  20,  20,  15,  10,  10,
		20,  25,  30,  35,  35,  30,  25,  20,
		40,  45,  50,  60,  60,  50,  45,  40,
		70,  80,  80,  80,  80,  80,  80,  70,
	   140, 150, 160, 160, 160, 160, 150, 140,
		 0,   0,   0,   0,   0,   0,   0,   0 };

	static constexpr int KnightPSQT[64] = {
		-50, -40, -30, -30, -30, -30, -40, -50,
		-40, -20,   0,   5,   5,   0, -20, -40,
		-30,   5,  10,  15,  15,  10,   5, -30,
		-30,   0,  15,  20,  20,  15,   0, -30,
		-30,   5,  15,  20,  20,  15,   5, -30,
		-30,   0,  10,  15,  15,  10,   0, -30,
		-40, -20,   0,   0,   0,   0, -20, -40,
		-50, -40, -30, -30, -30, -30, -40, -50
	};

	static constexpr int BishopPSQT[64] = {
		-20, -10, -10, -10, -10, -10, -10, -20,
		-10,   5,   0,   0,   0,   0,   5, -10,
		-10,  10,  10,  10,  10,  10,  10, -10,
		-10,   0,  10,  10,  10,  10,   0, -10,
		-10,   5,   5,  10,  10,   5,   5, -10,
		-10,   0,   5,  10,  10,   5,   0, -10,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-20, -10, -10, -10, -10, -10, -10, -20
	};

	static constexpr int RookPSQT[64] = {
		 0,  0,  0,  5,  5,  0,  0,  0,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		 5, 10, 10, 10, 10, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};

	static constexpr int QueenPSQT[64] = {
		-20, -10, -10,  -5,  -5, -10, -10, -20,
		-10,   0,   5,   0,   0,   0,   0, -10,
		-10,   5,   5,   5,   5,   5,   0, -10,
		  0,   0,   5,   5,   5,   5,   0,  -5,
		 -5,   0,   5,   5,   5,   5,   0,  -5,
		-10,   0,   5,   5,   5,   5,   0, -10,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-20, -10, -10,  -5,  -5, -10, -10, -20
	};

	static constexpr int KingEarlyPSQT[64] = {
		 20,  30,  10,   0,   0,  10,  30,  20,
		 20,  20,   0,   0,   0,   0,  20,  20,
		-10, -20, -20, -20, -20, -20, -20, -10,
		-20, -30, -30, -40, -40, -30, -30, -20,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30
	};

	static constexpr int KingLatePSQT[64] = {
		-50, -30, -30, -30, -30, -30, -30, -50,
		-30, -30,   0,   0,   0,   0, -30, -30,
		-30, -10,  20,  30,  30,  20, -10, -30,
		-30, -10,  30,  40,  40,  30, -10, -30,
		-30, -10,  30,  40,  40,  30, -10, -30,
		-30, -10,  20,  30,  30,  20, -10, -30,
		-30, -20, -10,   0,   0, -10, -20, -30,
		-50, -40, -30, -20, -20, -30, -40, -50
	};

};*/

static const int Weights[WeightsSize] = {

	// 1. Pawn early PSQT
	  0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,
	 10,  10,  15,  20,  20,  15,  10,  10,
	 20,  25,  30,  35,  35,  30,  25,  20,
	 40,  45,  50,  60,  60,  50,  45,  40,
	 70,  80,  80,  80,  80,  80,  80,  70,
	140, 150, 160, 160, 160, 160, 150, 140,
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
	 20,  30,  10,   0,   0,  10,  30,  20,
	 20,  20,   0,   0,   0,   0,  20,  20,
	-10, -20, -20, -20, -20, -20, -20, -10,
	-20, -30, -30, -40, -40, -30, -30, -20,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,
	-30, -40, -40, -50, -50, -40, -40, -30,

	// 13. Material early values
	100,
	300,
	300,
	500,
	900,
	0,

	// 14. Material late values
	100,
	300,
	300,
	500,
	900,
	0,

	// 15. Bishop pair bonus
	50,
};


const static int Mirror[] = {
	56, 57, 58, 59, 60, 61, 62, 63,
	48, 49, 50, 51, 52, 53, 54, 55,
	40, 41, 42, 43, 44, 45, 46, 47,
	32, 33, 34, 35, 36, 37, 38, 39,
	24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23,
	 8,  9, 10, 11, 12, 13, 14, 15,
	 0,  1,  2,  3,  4,  5,  6,  7
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

static const int CalculateGamePhase(Board &board) {
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
// 780: Bishop pair bonus


inline static constexpr int IndexEarlyPSQT(const int pieceType, const int location) {
	return (pieceType - 1) * 64 + location;
}

inline static constexpr int IndexLatePSQT(const int pieceType, const int location) {
	return (pieceType + 6) * 64 + location;
}

inline static constexpr int IndexPieceValueEarly(const int pieceType) {
	return 767 + pieceType;
}

inline static constexpr int IndexPieceValueLate(const int pieceType) {
	return 773 + pieceType;
}

inline static constexpr int IndexBishopPairBonus() {
	return 780;
}

inline static const int EvaluateBoard(Board& board, const int level, const int weights[WeightsSize]) {

	// 1. is over?
	if (board.State == GameState::Draw) return 0;
	if (board.State == GameState::WhiteVictory) {
		if (board.Turn == Turn::White) return MateEval - (level + 1) / 2;
		if (board.Turn == Turn::Black) return -MateEval + (level + 1) / 2;
	}
	else if (board.State == GameState::BlackVictory) {
		if (board.Turn == Turn::White) return -MateEval + (level + 1) / 2;
		if (board.Turn == Turn::Black) return MateEval - (level + 1) / 2;
	}

	// 2. Materials
	int score = 0;

	if (Popcount(board.WhiteBishopBits) >= 2) score += weights[IndexBishopPairBonus()];
	if (Popcount(board.BlackBishopBits) >= 2) score -= weights[IndexBishopPairBonus()];

	uint64_t occupancy = board.GetOccupancy();
	float phase = CalculateGamePhase(board);
	while (occupancy != 0) {
		uint64_t i = 64 - __lzcnt64(occupancy) - 1;
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
	}

	if (!board.Turn) score *= -1;
	return score;
}

inline static const int EvaluateBoard(Board& board, const int level) {
	return EvaluateBoard(board, level, Weights);
}
