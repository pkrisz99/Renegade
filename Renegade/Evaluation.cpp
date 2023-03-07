#pragma once
#include "Board.h"
#include "Move.h"

const static int WeightsSize = 931;

extern uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetQueenAttacks(const int square, const uint64_t occupancy);

// Source of values:
// https://www.chessprogramming.org/Simplified_Evaluation_Function

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
// 780+: Other

static const int Weights[WeightsSize] = {

	// 1. Pawn early PSQT
	  0,   0,   0,   0,   0,   0,   0,   0,
	  5,  10,  10, -20, -20,  10,  10,   5,
	  5,  -5, -10,   0,   0, -10,   5,   5,
	  0,   0,   0,  20,  20,   0,   0,   0,
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
	 -30, -30,   0,   0,   0,   0, -30, -30,
	 -30, -10,  20,  30,  30,  20, -10, -30,
	 -30, -10,  30,  40,  40,  30, -10, -30,
	 -30, -10,  30,  40,  40,  30, -10, -30,
	 -30, -10,  20,  30,  30,  20, -10, -30,
	 -30, -20, -10,   0,   0, -10, -20, -30,
	 -50, -40, -30, -20, -20, -30, -40, -50,

	// 13. Material early values
	 100, 305, 310, 500, 950, 0,

	// 14. Material late values
	 100, 280, 330, 500, 950, 0,

	// 15. Bishop pair bonus (early & late game)
	  30, 40,

	// 16. Tempo bonus (early & late game)
	  18, 0,

	// 17. Doubled and tripled pawn penalty (early doubled, late doubled, early tripled, late tripled)
	 -15, -20,
	 -30, -40,

	// 18. Passed pawn bonus (early & late game)
	   5, 25,

	// 19. Defended pawn bonus (early & late game) -- unused
	   0, 0,

	// 20. King safety weight -- replaced, see below
	 -84, // unused

	// 21. Knight mobility
	 -20, -15, -10, -5, 0, 5, 10, 15, 20,

	// 22. Bishop mobility
	 -36, -30, -24, -18, -12, -6, 0, 6, 12, 18, 24, 30, 36, 36,

	// 23. Rook mobility
	 -35, -30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30, 35,
	   0, //unused

	// 25. Queen mobility (early & late game)
	 -13, -12, -11, -10,  -9,  -8,  -7,  -6,  -5,  -4, -3, -2, -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 13,
	 -39, -36, -33, -30, -27, -24, -21, -18, -15, -12, -9, -6, -3,  0,  3,  6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 40,

	// 26. King safety score
	 -13, -18, -19, -34, -64, -82, -91, -130, -130, -160, -180, -200, -220, -240, -260, -280, -300, -320, -340, -360, -380, -400, -420, -440, -460,

	// 27. King safety dangers
	  1, 3, 2, 4, 6,

	// 28. King safety multipliers (/100)
	  50, 70, 80, 90, 95, 98, 100,

	// 29. Rook on open and semi open files (early, late)
	  20, 10,
	  10, 5,

	// 30. Knight outposts (early, late)
	  15, 7,
	  

	// +: pawn mobility, isolated pawns, doubled rooks...
};

// Weight indices ---------------------------------------------------------------------------------

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
const int IndexDefendedPawnEarly = 790;
const int IndexDefendedPawnLate = 791;
const int IndexPawnDanger = 913;
const int IndexKnightDanger = 914;
const int IndexBishopDanger = 915;
const int IndexRookDanger = 916;
const int IndexQueenDanger = 917;
const int IndexRookOpenFileEarly = 925;
const int IndexRookSemiOpenFileEarly = 926;
const int IndexRookOpenFileLate = 927;
const int IndexRookSemiOpenFileLate = 928;
const int IndexKnightOutpostEarly = 929;
const int IndexKnightOutpostLate = 930;

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

inline static constexpr int IndexKnightMobility(const int mobility) {
	return 793 + mobility;
}

inline static constexpr int IndexBishopMobility(const int mobility) {
	return 802 + mobility;
}

inline static constexpr int IndexRookMobility(const int mobility) {
	return 816 + mobility;
}

inline static constexpr int IndexQueenEarlyMobility(const int mobility) {
	return 832 + mobility;
}

inline static constexpr int IndexQueenLateMobility(const int mobility) {
	return 860 + mobility;
}

inline static constexpr int IndexKingSafety(const int danger) {
	return 887 + danger;
}

inline static constexpr int IndexDangerPieces(const int danger) {
	return 917 + danger;
}

// Interpolation functions ------------------------------------------------------------------------

static inline int LinearTaper(const int earlyValue, const int lateValue, const float phase) {
	return (int)((1 - phase) * (float)earlyValue + phase * (float)lateValue);
}

// Game phase calculation -------------------------------------------------------------------------

static const float CalculateGamePhase(Board& board) {
	int remainingPawns = Popcount(board.WhitePawnBits | board.BlackPawnBits);
	int remainingKnights = Popcount(board.WhiteKnightBits | board.BlackKnightBits);
	int remainingBishops = Popcount(board.WhiteBishopBits | board.BlackBishopBits);
	int remainingRooks = Popcount(board.WhiteRookBits | board.BlackRookBits);
	int remainingQueens = Popcount(board.WhiteQueenBits | board.BlackQueenBits);

	// starting: 16 + 12 + 12 + 24 + 24 = 88
	// 88 -> 0, 10 -> 1
	int remainingScore = remainingPawns * 1 + remainingKnights * 3 + remainingBishops * 3 + remainingRooks * 6 + remainingQueens * 12;

	float phase = (88 - remainingScore) / (88.f - 10.f);
	return std::clamp(phase, 0.f, 1.f);
}

// Board evaluation -------------------------------------------------------------------------------

inline static const int EvaluateBoard(Board& board, const int level, const int weights[WeightsSize]) {

	int score = 0;
	const uint64_t occupancy = board.GetOccupancy();
	uint64_t piecesOnBoard = occupancy;
	uint64_t whitePieces = board.GetOccupancy(PieceColor::White);
	uint64_t blackPieces = board.GetOccupancy(PieceColor::Black);
	float phase = CalculateGamePhase(board);

	int mobilityScore = 0;
	uint64_t allOccupancy = occupancy;
	std::tuple<int, uint64_t> mob;


	int whiteDangerScore = 0;
	int blackDangerScore = 0;
	int whiteDangerPieces = 0;
	int blackDangerPieces = 0;
	const int dangerWeights[] = { 0, weights[IndexPawnDanger], weights[IndexKnightDanger], weights[IndexBishopDanger], weights[IndexRookDanger], weights[IndexQueenDanger], 4 };

	int whiteKingSquare = 63 - Lzcount(board.WhiteKingBits);
	uint64_t whiteKingZone = KingArea[whiteKingSquare];
	int blackKingSquare = 63 - Lzcount(board.BlackKingBits);
	uint64_t blackKingZone = KingArea[blackKingSquare];

	while (piecesOnBoard != 0) {
		int i = 63 - Lzcount(piecesOnBoard);
		SetBitFalse(piecesOnBoard, i);
		int piece = board.GetPieceAt(i);
		int pieceType = TypeOfPiece(piece);
		int pieceColor = ColorOfPiece(piece);

		if (pieceColor == PieceColor::White) {
			int psqt = LinearTaper(weights[IndexEarlyPSQT(pieceType, i)], weights[IndexLatePSQT(pieceType, i)], phase);
			score += LinearTaper(weights[IndexPieceValueEarly(pieceType)], weights[IndexPieceValueLate(pieceType)], phase);
			score += psqt;
		}
		else if (pieceColor == PieceColor::Black) {
			int psqt = LinearTaper(weights[IndexEarlyPSQT(pieceType, Mirror[i])], weights[IndexLatePSQT(pieceType, Mirror[i])], phase);
			score -= LinearTaper(weights[IndexPieceValueEarly(pieceType)], weights[IndexPieceValueLate(pieceType)], phase);
			score -= psqt;
		}

		// Passed pawn bonus
		if (piece == Piece::WhitePawn) {
			if (((WhitePassedPawnMask[i] & board.BlackPawnBits) == 0) && ((WhitePassedPawnFilter[i] & board.WhitePawnBits) == 0))
				score += LinearTaper(weights[IndexPassedPawnEarly], weights[IndexPassedPawnLate], phase);
		}
		else if (piece == Piece::BlackPawn) {
			if (((BlackPassedPawnMask[i] & board.WhitePawnBits) == 0) && ((BlackPassedPawnFilter[i] & board.BlackPawnBits) == 0))
				score -= LinearTaper(weights[IndexPassedPawnEarly], weights[IndexPassedPawnLate], phase);
		}
		// Knight outposts (be careful with OutpostFilter, the following lines assume edges aren't outposts)
		else if ((piece == Piece::WhiteKnight) && OutpostFilter[i]) {
			if ((board.GetPieceAt(i - 7) == Piece::WhitePawn) || (board.GetPieceAt(i - 9) == Piece::WhitePawn)) {
				score += LinearTaper(weights[IndexKnightOutpostEarly], weights[IndexKnightOutpostLate], phase);
			}
		} else if ((piece == Piece::BlackKnight) && OutpostFilter[i]) {
			if ((board.GetPieceAt(i + 7) == Piece::BlackPawn) || (board.GetPieceAt(i + 9) == Piece::BlackPawn)) {
				score -= LinearTaper(weights[IndexKnightOutpostEarly], weights[IndexKnightOutpostLate], phase);
			}
		}
		// Rooks on (semi) open files
		else if (piece == Piece::WhiteRook) {
			int file = GetSquareFile(i);
			if ((board.WhitePawnBits & Files[file]) == 0) {
				score += ((board.BlackPawnBits & Files[file]) == 0) ? 
					LinearTaper(weights[IndexRookOpenFileEarly], weights[IndexRookOpenFileLate], phase) : // Open file
					LinearTaper(weights[IndexRookSemiOpenFileEarly], weights[IndexRookSemiOpenFileLate], phase); // Semi-open file
			}
		}
		else if (piece == Piece::BlackRook) {
			int file = GetSquareFile(i);
			if ((board.BlackPawnBits & Files[file]) == 0) {
				score -= ((board.WhitePawnBits & Files[file]) == 0) ?
					LinearTaper(weights[IndexRookOpenFileEarly], weights[IndexRookOpenFileLate], phase) : // Open file
					LinearTaper(weights[IndexRookSemiOpenFileEarly], weights[IndexRookSemiOpenFileLate], phase); // Semi-open file
			}
		}

		// Mobility - todo: make this prettier
		uint64_t mobility = 0, attacks = 0;
		int earlyScore = 0, lateScore = 0;
		switch (piece) {
		case Piece::WhitePawn:
			attacks = WhitePawnAttacks[i] & ~whitePieces;
			if ((blackKingZone & attacks) != 0) {
				blackDangerScore += dangerWeights[PieceType::Pawn];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackPawn:
			attacks = BlackPawnAttacks[i] & ~blackPieces;
			if ((whiteKingZone & attacks) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Pawn];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteKnight:
			mobility = KnightMoveBits[i] & ~whitePieces;
			mobilityScore += weights[IndexKnightMobility(Popcount(mobility))];
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Knight];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackKnight:
			mobility = KnightMoveBits[i] & ~blackPieces;
			mobilityScore -= weights[IndexKnightMobility(Popcount(mobility))];
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Knight];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteBishop:
			mobility = GetBishopAttacks(i, occupancy) & ~whitePieces;
			mobilityScore += weights[IndexBishopMobility(Popcount(mobility))];
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Bishop];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackBishop:
			mobility = GetBishopAttacks(i, occupancy) & ~blackPieces;
			mobilityScore -= weights[IndexBishopMobility(Popcount(mobility))];
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Bishop];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteRook:
			mobility = GetRookAttacks(i, occupancy) & ~whitePieces;
			mobilityScore += weights[IndexRookMobility(Popcount(mobility))];
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Rook];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackRook:
			mobility = GetRookAttacks(i, occupancy) & ~blackPieces;
			mobilityScore -= weights[IndexRookMobility(Popcount(mobility))];
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Rook];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteQueen:
			mobility = GetQueenAttacks(i, occupancy) & ~whitePieces;
			earlyScore = weights[IndexQueenEarlyMobility(Popcount(mobility))];
			lateScore = weights[IndexQueenLateMobility(Popcount(mobility))];
			mobilityScore += LinearTaper(earlyScore, lateScore, phase);
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Queen];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackQueen:
			mobility = GetQueenAttacks(i, occupancy) & ~blackPieces;
			earlyScore = weights[IndexQueenEarlyMobility(Popcount(mobility))];
			lateScore = weights[IndexQueenLateMobility(Popcount(mobility))];
			mobilityScore -= LinearTaper(earlyScore, lateScore, phase);
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Queen];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteKing:
			break;
		case Piece::BlackKing:
			break;
		}
	}

	// Taper mobility contribution to eval score
	// To avoid putting the queen out too early
	score += LinearTaper(0, mobilityScore, std::min(phase * 7.f, 1.f));

	// King safety
	// Todo idea: more severe penalty if king is in the corner
	const int dangerPieces[] = { 0, weights[IndexDangerPieces(1)], weights[IndexDangerPieces(2)], weights[IndexDangerPieces(3)], 
		weights[IndexDangerPieces(4)], weights[IndexDangerPieces(5)], weights[IndexDangerPieces(6)], weights[IndexDangerPieces(7)] };
	int whiteKingSafety = whiteDangerScore * dangerPieces[std::min(whiteDangerPieces, 7)] / 100;
	int blackKingSafety = blackDangerScore * dangerPieces[std::min(blackDangerPieces, 7)] / 100;
	if (whiteKingSafety != 0) score += weights[IndexKingSafety(std::min(whiteKingSafety, 25))];
	if (blackKingSafety != 0) score -= weights[IndexKingSafety(std::min(blackKingSafety, 25))];


	// Bishop pair bonus
	if (Popcount(board.WhiteBishopBits) >= 2) score += LinearTaper(weights[IndexBishopPairEarly], weights[IndexBishopPairLate], phase);
	if (Popcount(board.BlackBishopBits) >= 2) score -= LinearTaper(weights[IndexBishopPairEarly], weights[IndexBishopPairLate], phase);

	// Doubled pawn penalties
	for (int i = 0; i < 8; i++) {
		const int whitePawnsOnFile = Popcount(board.WhitePawnBits & Files[i]);
		const int blackPawnsOnFile = Popcount(board.BlackPawnBits & Files[i]);
		if (whitePawnsOnFile == 2) score += LinearTaper(weights[IndexDoubledPawnEarly], weights[IndexDoubledPawnLate], phase);
		if (whitePawnsOnFile > 2) score += LinearTaper(weights[IndexTripledPawnEarly], weights[IndexTripledPawnLate], phase);
		if (blackPawnsOnFile == 2) score -= LinearTaper(weights[IndexDoubledPawnEarly], weights[IndexDoubledPawnLate], phase);
		if (blackPawnsOnFile > 2) score -= LinearTaper(weights[IndexTripledPawnEarly], weights[IndexTripledPawnLate], phase);
	}

	// Pawns defending pawns - currently unused
	/*
	int whiteDefendedPawns = Popcount((board.WhitePawnBits & ~Bitboards::FileA & (board.WhitePawnBits << 7)) | (board.WhitePawnBits & ~Bitboards::FileH & (board.WhitePawnBits << 9)));
	int blackDefendedPawns = Popcount((board.BlackPawnBits & ~Bitboards::FileA & (board.BlackPawnBits >> 9)) | (board.BlackPawnBits & ~Bitboards::FileH & (board.BlackPawnBits >> 7)));
	const int defendingBonus = LinearTaper(weights[IndexDefendedPawnEarly], weights[IndexDefendedPawnLate], phase);
	score += defendingBonus * whiteDefendedPawns;
	score -= defendingBonus * blackDefendedPawns; */

	// Convert to the correct perspective
	if (!board.Turn) score *= -1;

	// Tempo bonus
	score += LinearTaper(weights[IndexTempoEarly], weights[IndexTempoLate], phase);

	// Drawish endgame detection
	// To avoid simplifying down to a non-winning endgame with a nominal material advantage
	// This list is not complete, and probably should be expanded even more (e.g. by including pawns)
	bool potentiallyDrawish = false;
	if ((whitePieces <= 3) && (blackPieces <= 3)) {
		if (Popcount(board.WhiteQueenBits | board.BlackQueenBits | board.WhitePawnBits | board.BlackPawnBits) == 0) potentiallyDrawish = true;
	}
	if (potentiallyDrawish) {
		uint64_t whiteExtras = whitePieces - 1;
		uint64_t blackExtras = blackPieces - 1;
		uint64_t whiteMinors = Popcount(board.WhiteKnightBits | board.WhiteBishopBits);
		uint64_t blackMinors = Popcount(board.BlackKnightBits | board.BlackBishopBits);
		uint64_t whiteKnights = Popcount(board.WhiteKnightBits);
		uint64_t blackKnights = Popcount(board.BlackKnightBits);
		uint64_t whiteBishops = Popcount(board.WhiteBishopBits);
		uint64_t blackBishops = Popcount(board.BlackBishopBits);
		bool drawish =
			// 2 minor pieces (no bishop pair) vs 1 minor piece
			((whiteExtras == 2) && (whiteMinors == 2) && (whiteBishops != 2) && (blackExtras == 1) && (blackMinors == 1)) ||
			((blackExtras == 2) && (blackMinors == 2) && (blackBishops != 2) && (whiteExtras == 1) && (whiteMinors == 1)) ||
			// 2 knights vs king
			((whiteExtras == 0) && (blackExtras == 2) && (blackKnights == 2)) ||
			((blackExtras == 0) && (whiteExtras == 2) && (whiteKnights == 2)) ||
			// minor piece vs minor piece
			((whiteExtras == 1) && (whiteMinors == 1) && (blackExtras == 1) && (blackMinors == 1)) ||
			// minor piece vs 2 knights
			((whiteExtras == 1) && (whiteMinors == 1) && (blackExtras == 2) && (blackKnights == 2)) ||
			((blackExtras == 1) && (blackMinors == 1) && (whiteExtras == 2) && (whiteKnights == 2)) ||
			// 2 bishop vs 1 bishop
			((whiteExtras == 2) && (whiteBishops == 2) && (blackExtras == 1) && (blackBishops == 1)) ||
			((blackExtras == 2) && (blackBishops == 2) && (whiteExtras == 1) && (whiteBishops == 1));
		if (drawish) score = score / 16;
	}

	return score;
}

inline static const int EvaluateBoard(Board& board, const int level) {
	return EvaluateBoard(board, level, Weights);
}
