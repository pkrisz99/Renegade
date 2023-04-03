#pragma once
#include "Board.h"
#include "Move.h"

// Yes, I know, it looks terrible

const static int WeightsSize = 953;

extern uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetQueenAttacks(const int square, const uint64_t occupancy);


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
	0, 0, 0, 0, 0, 0, 0, 0, -19, -18, -15, -20, -6, 16, 27, -7, -19, -15, -9, -12, -2, 2, 23, 4, -24, -18, -9, 8, 3, 8, 4, -10, -16, -12, -2, 5, 27, 24, 19, 4, 18, 11, 31, 46, 51, 77, 56, 32, 96, 105, 72, 106, 65, 68, -21, -21, 0, 0, 0, 0, 0, 0, 0, 0,

	// 2. Knight early PSQT
	-80, -40, -33, -27, -20, -19, -43, -52, -45, -35, -22, -10, -10, -1, -21, -20, -36, -17, 0, 13, 21, 2, 0, -24, -23, -8, 11, 16, 23, 15, 16, -9, -5, -1, 24, 50, 24, 52, 8, 19, -15, 29, 59, 70, 75, 70, 50, -7, -51, 1, 29, 31, 25, 60, 31, 20, -110, -100, -39, -32, -26, -80, -80, -110,

	// 3. Bishop early PSQT
	-8, 14, -7, -2, -3, -12, 18, 0, 2, 5, 16, -8, 5, 14, 17, 1, -2, 4, 7, 7, 10, 8, 5, 10, -18, -15, -4, 26, 17, 0, 0, 2, -13, 0, 16, 34, 27, 26, 4, -4, -10, 19, 26, 21, 15, 45, 49, 21, -21, 16, 2, -7, 16, 9, 20, -5, -4, -70, -9, -70, -70, -70, -46, -43,

	// 4. Rook early PSQT
	-16, -13, -6, 7, 6, 5, 4, -17, -35, -18, -17, -13, -11, 2, 9, -22, -25, -25, -23, -17, -8, -15, 9, -3, -28, -26, -11, -15, -8, -24, -10, -21, -21, 3, -10, 2, 1, 8, 8, 2, -6, 14, 11, 4, 31, 33, 64, 34, -9, 1, 4, 15, -1, 36, 24, 16, -18, -4, 13, -13, -5, 6, 32, 28,

	// 5. Queen early PSQT
	-2, -7, 0, 6, 4, -3, -5, -6, -7, -4, 6, 11, 10, 18, 6, 26, -12, -5, -1, -6, -6, 2, 0, -3, -12, -10, -13, -12, -12, -9, 0, -4, -18, -6, -11, -20, 1, 1, 4, 9, -1, -14, 0, -15, 12, 44, 60, 35, -17, -18, -3, -8, 9, 13, 11, 51, -34, -19, -8, -22, 3, 19, 45, 8,

	// 6. King early PSQT
	  33, 59, 32, -40, 6, -25, 42, 39, 51, 17, 4, -27, -27, -18, 20, 30, -18, 3, -27, -43, -31, -43, -16, -43, -23, -26, -41, -54, -67, -22, -28, -56, -13, -14, -34, -53, -49, -25, -12, -20, -36, 0, -30, -30, -23, 0, 0, 1, -37, -13, -42, -13, -26, -5, 0, 10, -51, -37, -29, -43, -34, -4, 0, -29,


	// 7. Pawn late PSQT
	0, 0, 0, 0, 0, 0, 0, 0, 17, 30, 19, 32, 31, 21, 14, 4, 12, 23, 12, 24, 23, 18, 14, 0, 23, 29, 13, 5, 7, 10, 16, 6, 49, 50, 31, 18, 12, 19, 35, 29, 115, 138, 110, 81, 67, 65, 114, 100, 178, 170, 179, 144, 154, 128, 186, 193, 0, 0, 0, 0, 0, 0, 0, 0,

	// 8. Knight late PSQT
	-37, -22, -11, -1, -8, -17, -6, -21, -14, -4, 4, 6, 6, 0, -20, -11, -18, 1, 5, 18, 21, 4, -1, -15, 7, 12, 27, 20, 27, 17, 5, -4, -9, 14, 27, 24, 21, 19, 13, -3, 4, -1, 13, 10, 11, 1, -13, -14, -9, -6, -11, -4, -11, 2, -28, -19, -88, -38, -17, -23, -8, -26, -19, -89,

	// 9. Bishop late PSQT
	-17, 0, 5, -9, -5, 10, -12, -22, -13, -11, -16, 4, -4, -3, -3, -10, -5, -7, 1, -2, 6, 2, 0, -14, -12, 3, 4, -3, 7, -4, -5, -32, -12, -6, 3, 4, -4, -1, -4, -13, -3, -8, -11, -12, -18, -12, -12, -10, -13, -11, -20, -9, -30, -20, -9, -25, -4, -2, -14, -9, -19, -16, -11, -18,

	// 10. Rook late PSQT
	9, 7, 6, 4, -3, -1, -6, -3, 3, -1, 5, 3, -4, -12, -10, -8, 5, 6, 5, 2, -4, -12, -17, -19, 6, 8, 13, 14, 2, 1, 3, 2, 16, 8, 17, 12, -5, -6, 2, -6, 14, 11, 7, 12, -6, -4, 2, 1, 20, 21, 26, 12, 11, 9, 4, 16, 29, 11, 21, 20, 18, 16, 8, 12,

	// 11. Queen late PSQT
	-11, -12, -16, 10, -5, -19, -22, -16, -15, -16, -12, -2, 3, -21, -19, -68, -3, 2, 7, 10, 25, 5, 12, 20, -5, -3, 5, 34, 31, 26, 7, 31, 14, 11, 10, 35, 27, 42, 33, 15, 6, 21, 34, 36, 34, 41, 0, 40, 16, 10, 34, 44, 56, 26, 19, 27, 15, -6, 17, 45, 21, 21, -3, 20,

	// 12. King late PSQT
	 -82, -60, -43, -33, -44, -32, -55, -86, -47, -23, -11, -2, 3, -2, -20, -40, -34, -11, 9, 24, 23, 18, -2, -17, -34, 2, 29, 43, 46, 30, 11, -7, -17, 15, 33, 42, 45, 43, 26, -4, -10, 10, 35, 40, 46, 53, 30, 2, -25, 4, 10, 16, 22, 27, 20, -7, -84, -46, -28, -6, -23, -11, 0, -90,


	// 13. Material early values
	 85, 325, 330, 444, 998, 0,

	// 14. Material late values
	 80, 289, 318, 560, 1016, 0,

	// 15. Bishop pair bonus (early & late game)
	  34, 55,

	// 16. Tempo bonus (early & late game)
	  18, 0, // 24, 16,

	// 17. Doubled and tripled pawn penalty (early doubled, late doubled, early tripled, late tripled)
	 -7, -20,
	 -12, -37,

	// 18. Passed pawn bonus (early & late game)
	   0, 31,

	// 19. Defended pawn bonus (early & late game) -- unused
	   0, 0,

	// 20. King safety weight -- replaced, see below
	 -84, // unused

	// 21. Knight mobility
	-21, -6, 2, 5, 9, 11, 11, 11, 10,

	// 22. Bishop mobility
	 -45, -34, -22, -16, -7, 3, 9, 13, 16, 14, 14, 16, 16, 8,

	// 23. Rook mobility
	-29, -16, -12, -6, -4, 1, 4, 6, 9, 13, 13, 14, 16, 17, 17,
	   0, //unused

	// 25. Queen mobility (early & late game)
	 -20, -11, -31, -21, -19, -14, -10, -9, -5, -2, -2, 0, 3, -1, 3, 4, 3, 2, 6, 16, 25, 25, 19, 33, 28, 37, 20, 78,
	 -55, -80, -39, -37, -44, -33, -14, -29, -13, -17, 2, -4, 2, 15, 20, 27, 22, 42, 47, 48, 51, 51, 54, 47, 50, 56, 46, 75,

	// 26. King safety score
	// -4, -8, -13, -18, -30, -44, -60, -80, -105, -130, -160, -190, -220, -240, -260, -280, -300, -320, -340, -360, -380, -400, -420, -440, -460,
	  -5, -5, -20, -25, -36, -56, -72, -90, -133, -190, -222, -252, -255, -178, -322, -332, -350, -370, -400, -422, -425, -430, -435, -440, -445,

	// 27. King safety dangers
	  1, 2, 2, 3, 5,

	// 28. King safety multipliers (/100)
	  50, 70, 80, 90, 95, 98, 100,

	// 29. Rook on open and semi open files (early, late)
	  27, 12,
	  9, 9,

	// 30. Knight outposts (early, late)
	  1, 13,
	  
	// 31. Isolated pawn penalties by file (early, late)
		 -13, -12, -13, -17, -18, -20, -6, -5,
		 1, -9, -14, -20, -18, -8, -16, -3,

	// 32. Pawn attacking minors (early, late) and majors (early, late)
	 51, 28,
	 56, 32,

	// 33. Minor pieces attacking majors (early, late)
	 8, 10,

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
const int IndexPawnAttackingMinorEarly = 947;
const int IndexPawnAttackingMinorLate = 948;
const int IndexPawnAttackingMajorEarly = 949;
const int IndexPawnAttackingMajorLate = 950;
const int IndexMinorAttackingMajorEarly = 951;
const int IndexMinorAttackingMajorLate = 952;

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

inline static constexpr int IndexIsolatedPawnPenaltyEarly(const int file) {
	return 931 + file;
}

inline static constexpr int IndexIsolatedPawnPenaltyLate(const int file) {
	return 939 + file;
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

	int remainingScore = remainingPawns + remainingKnights * 10 + remainingBishops * 10 + remainingRooks * 20 + remainingQueens * 40;

	float phase = (256 - remainingScore) / (256.f);
	return std::clamp(phase, 0.f, 1.f);
}

// Board evaluation -------------------------------------------------------------------------------

inline static const int EvaluateBoard(Board& board, const int level, const int weights[WeightsSize]) {

	int score = 0, earlyScore = 0, lateScore = 0;

	const uint64_t occupancy = board.GetOccupancy();
	uint64_t piecesOnBoard = occupancy;
	uint64_t whitePieces = board.GetOccupancy(PieceColor::White);
	uint64_t blackPieces = board.GetOccupancy(PieceColor::Black);
	float phase = CalculateGamePhase(board);

	int mobilityScore = 0;
	uint64_t allOccupancy = occupancy;
	std::tuple<int, uint64_t> mob;
	uint64_t whiteAttacks = 0, blackAttacks = 0;

	int whiteDangerScore = 0;
	int blackDangerScore = 0;
	int whiteDangerPieces = 0;
	int blackDangerPieces = 0;
	const int dangerWeights[] = { 0, weights[IndexPawnDanger], weights[IndexKnightDanger], weights[IndexBishopDanger], weights[IndexRookDanger], weights[IndexQueenDanger], 4 };

	int whiteKingSquare = 63 - Lzcount(board.WhiteKingBits);
	int whiteKingFile = GetSquareFile(whiteKingSquare);
	int whiteKingRank = GetSquareRank(whiteKingSquare);
	uint64_t whiteKingZone = KingArea[whiteKingSquare];
	int blackKingSquare = 63 - Lzcount(board.BlackKingBits);
	int blackKingFile = GetSquareFile(blackKingSquare);
	int blackKingRank = GetSquareRank(blackKingSquare);
	uint64_t blackKingZone = KingArea[blackKingSquare];

	uint64_t whiteMajorBits = board.WhiteRookBits | board.WhiteQueenBits;
	uint64_t whiteMinorBits = board.WhiteKnightBits | board.WhiteBishopBits;
	uint64_t blackMajorBits = board.BlackRookBits | board.BlackQueenBits;
	uint64_t blackMinorBits = board.BlackKnightBits | board.BlackBishopBits;

	while (piecesOnBoard != 0) {
		int i = 63 - Lzcount(piecesOnBoard);
		SetBitFalse(piecesOnBoard, i);
		int piece = board.GetPieceAt(i);
		int pieceType = TypeOfPiece(piece);
		int pieceColor = ColorOfPiece(piece);
		int file = GetSquareFile(i);

		// Material and piece-square tables
		if (pieceColor == PieceColor::White) {
			earlyScore += weights[IndexEarlyPSQT(pieceType, i)];
			lateScore += weights[IndexLatePSQT(pieceType, i)];
			earlyScore += weights[IndexPieceValueEarly(pieceType)];
			lateScore += weights[IndexPieceValueLate(pieceType)];
		}
		else if (pieceColor == PieceColor::Black) {
			earlyScore -= weights[IndexEarlyPSQT(pieceType, Mirror[i])];
			lateScore -= weights[IndexLatePSQT(pieceType, Mirror[i])];
			earlyScore -= weights[IndexPieceValueEarly(pieceType)];
			lateScore -= weights[IndexPieceValueLate(pieceType)];
		}

		// Piece-specific evaluation
		uint64_t mobility = 0, attacks = 0;
		int early = 0, late = 0;
		switch (piece) {
		case Piece::WhitePawn:
			attacks = WhitePawnAttacks[i] & ~whitePieces;
			whiteAttacks |= attacks;
			if ((blackKingZone & attacks) != 0) {
				blackDangerScore += dangerWeights[PieceType::Pawn];
				blackDangerPieces += 1;
			}
			if ((board.WhitePawnBits & IsolatedPawnMask[file]) == 0) {
				earlyScore += weights[IndexIsolatedPawnPenaltyEarly(file)];
				lateScore += weights[IndexIsolatedPawnPenaltyLate(file)];
			}

			if (((WhitePassedPawnMask[i] & board.BlackPawnBits) == 0) && ((WhitePassedPawnFilter[i] & board.WhitePawnBits) == 0)) {
				earlyScore += weights[IndexPassedPawnEarly];
				lateScore += weights[IndexPassedPawnLate];
			}
			
			if (((GetSquareFile(i) != 0) && CheckBit(blackMajorBits, i + 7ULL))
				|| ((GetSquareFile(i) != 7) && CheckBit(blackMajorBits, i + 9ULL))) {
				earlyScore += weights[IndexPawnAttackingMajorEarly];
				lateScore += weights[IndexPawnAttackingMajorLate];
			}
			else if (((GetSquareFile(i) != 0) && CheckBit(blackMinorBits, i + 7ULL)) ||
				((GetSquareFile(i) != 7) && CheckBit(blackMinorBits, i + 9ULL))) {
				earlyScore += weights[IndexPawnAttackingMinorEarly];
				lateScore += weights[IndexPawnAttackingMinorLate];
			}

			break;
		case Piece::BlackPawn:
			attacks = BlackPawnAttacks[i] & ~blackPieces;
			blackAttacks |= attacks;
			if ((whiteKingZone & attacks) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Pawn];
				whiteDangerPieces += 1;
			}
			if ((board.BlackPawnBits & IsolatedPawnMask[file]) == 0) {
				earlyScore -= weights[IndexIsolatedPawnPenaltyEarly(7 - file)];
				lateScore -= weights[IndexIsolatedPawnPenaltyLate(7 - file)];
			}

			if (((BlackPassedPawnMask[i] & board.WhitePawnBits) == 0) && ((BlackPassedPawnFilter[i] & board.BlackPawnBits) == 0)) {
				earlyScore -= weights[IndexPassedPawnEarly];
				lateScore -= weights[IndexPassedPawnLate];
			}

			if (((GetSquareFile(i) != 0) && CheckBit(whiteMajorBits, i - 9ULL))
				|| ((GetSquareFile(i) != 7) && CheckBit(whiteMajorBits, i - 7ULL))) {
				earlyScore -= weights[IndexPawnAttackingMajorEarly];
				lateScore -= weights[IndexPawnAttackingMajorLate];
			}
			else if (((GetSquareFile(i) != 0) && CheckBit(whiteMinorBits, i - 9ULL)) ||
				((GetSquareFile(i) != 7) && CheckBit(whiteMinorBits, i - 7ULL))) {
				earlyScore -= weights[IndexPawnAttackingMinorEarly];
				lateScore -= weights[IndexPawnAttackingMinorLate];
			}
			break;

		case Piece::WhiteKnight:
			mobility = KnightMoveBits[i] & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights[IndexKnightMobility(Popcount(mobility))];
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Knight];
				blackDangerPieces += 1;
			}
			if (OutpostFilter[i]) {
				if ((board.GetPieceAt(i - 7) == Piece::WhitePawn) || (board.GetPieceAt(i - 9) == Piece::WhitePawn)) {
					earlyScore += weights[IndexKnightOutpostEarly];
					lateScore += weights[IndexKnightOutpostLate];
				}
			}

			break;
		case Piece::BlackKnight:
			mobility = KnightMoveBits[i] & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights[IndexKnightMobility(Popcount(mobility))];
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Knight];
				whiteDangerPieces += 1;
			}
			if (OutpostFilter[i]) {
				if ((board.GetPieceAt(i + 7) == Piece::BlackPawn) || (board.GetPieceAt(i + 9) == Piece::BlackPawn)) {
					earlyScore -= weights[IndexKnightOutpostEarly];
					lateScore -= weights[IndexKnightOutpostLate];
				}
			}
			break;

		case Piece::WhiteBishop:
			mobility = GetBishopAttacks(i, occupancy) & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights[IndexBishopMobility(Popcount(mobility))];
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Bishop];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackBishop:
			mobility = GetBishopAttacks(i, occupancy) & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights[IndexBishopMobility(Popcount(mobility))];
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Bishop];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteRook:
			mobility = GetRookAttacks(i, occupancy) & ~whitePieces;
			whiteAttacks |= mobility;
			mobilityScore += weights[IndexRookMobility(Popcount(mobility))];
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Rook];
				blackDangerPieces += 1;
			}
			if ((board.WhitePawnBits & Files[file]) == 0) {
				score += ((board.BlackPawnBits & Files[file]) == 0) ?
					LinearTaper(weights[IndexRookOpenFileEarly], weights[IndexRookOpenFileLate], phase) : // Open file
					LinearTaper(weights[IndexRookSemiOpenFileEarly], weights[IndexRookSemiOpenFileLate], phase); // Semi-open file
			}
			break;
		case Piece::BlackRook:
			mobility = GetRookAttacks(i, occupancy) & ~blackPieces;
			blackAttacks |= mobility;
			mobilityScore -= weights[IndexRookMobility(Popcount(mobility))];
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += dangerWeights[PieceType::Rook];
				whiteDangerPieces += 1;
			}
			if ((board.BlackPawnBits & Files[file]) == 0) {
				score -= ((board.WhitePawnBits & Files[file]) == 0) ?
					LinearTaper(weights[IndexRookOpenFileEarly], weights[IndexRookOpenFileLate], phase) : // Open file
					LinearTaper(weights[IndexRookSemiOpenFileEarly], weights[IndexRookSemiOpenFileLate], phase); // Semi-open file
			}
			break;

		case Piece::WhiteQueen:
			mobility = GetQueenAttacks(i, occupancy) & ~whitePieces;
			whiteAttacks |= mobility;
			early = weights[IndexQueenEarlyMobility(Popcount(mobility))];
			late = weights[IndexQueenLateMobility(Popcount(mobility))];
			mobilityScore += LinearTaper(early, late, phase);
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += dangerWeights[PieceType::Queen];
				blackDangerPieces += 1;
			}
			break;
		case Piece::BlackQueen:
			mobility = GetQueenAttacks(i, occupancy) & ~blackPieces;
			blackAttacks |= mobility;
			early = weights[IndexQueenEarlyMobility(Popcount(mobility))];
			late = weights[IndexQueenLateMobility(Popcount(mobility))];
			mobilityScore -= LinearTaper(early, late, phase);
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

	score += LinearTaper(earlyScore, lateScore, phase);

	// Taper mobility contribution to eval score
	// To avoid putting the queen out too early
	score += LinearTaper(mobilityScore / 3, mobilityScore, std::min(phase * 5.f, 1.f));

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
	// Source: Chess Programming Wiki + https://www.madchess.net/2021/04/08/madchess-3-0-beta-4d22dec-endgame-eval-scaling/
	bool endgame = (Popcount(whitePieces) <= 3) && (Popcount(blackPieces) <= 3);
	if (endgame) {
		// Variables for easy access
		bool pawnless = (board.WhitePawnBits | board.BlackPawnBits) == 0ULL;
		bool queenless = (board.WhiteQueenBits | board.BlackQueenBits) == 0ULL;
		bool queenful = (Popcount(board.WhiteQueenBits | board.BlackQueenBits) > 0)
			&& (Popcount(board.WhiteQueenBits) <= 1) && (Popcount(board.BlackQueenBits) <= 1);
		bool potentiallyDrawishQueenless = queenless && pawnless && endgame;
		bool potentiallyDrawishQueenful = queenful && pawnless && endgame;
		int whiteExtras = Popcount(whitePieces) - 1;
		int blackExtras = Popcount(blackPieces) - 1;
		int whiteMinors = Popcount(board.WhiteKnightBits | board.WhiteBishopBits);
		int blackMinors = Popcount(board.BlackKnightBits | board.BlackBishopBits);
		int whiteKnights = Popcount(board.WhiteKnightBits);
		int blackKnights = Popcount(board.BlackKnightBits);
		int whiteBishops = Popcount(board.WhiteBishopBits);
		int blackBishops = Popcount(board.BlackBishopBits);
		int whiteRooks = Popcount(board.WhiteRookBits);
		int blackRooks = Popcount(board.BlackRookBits);

		// Endgames with no queens
		if (potentiallyDrawishQueenless) {
			bool drawish =
				// 2 minor pieces (no bishop pair) vs 1 minor piece
				((whiteExtras == 2) && (blackExtras == 1) && (whiteMinors == 2) && (whiteBishops != 2) && (blackMinors == 1)) ||
				((whiteExtras == 1) && (blackExtras == 2) && (blackMinors == 2) && (whiteMinors == 1) && (blackBishops != 2)) ||
				// 2 knights vs king
				((whiteExtras == 0) && (blackExtras == 2) && (blackKnights == 2)) ||
				((whiteExtras == 2) && (blackExtras == 0) && (whiteKnights == 2)) ||
				// minor piece vs minor piece
				((whiteExtras == 1) && (blackExtras == 1) && (whiteMinors == 1) && (blackMinors == 1)) ||
				// minor piece vs 2 knights
				((whiteExtras == 1) && (blackExtras == 2) && (whiteMinors == 1) && (blackKnights == 2)) ||
				((whiteExtras == 2) && (blackExtras == 1) && (whiteKnights == 2) && (blackMinors == 1)) ||
				// 2 bishop vs 1 bishop
				((whiteExtras == 2) && (blackExtras == 1) && (whiteBishops == 2) && (blackBishops == 1)) ||
				((whiteExtras == 1) && (blackExtras == 2) && (whiteBishops == 1) && (blackBishops == 2)) ||
				// rook vs rook
				((whiteExtras == 1) && (blackExtras == 1) && (whiteRooks == 1) && (blackRooks == 1)) ||
				// 2 rooks vs 2 rooks
				((whiteExtras == 1) && (blackExtras == 1) && (whiteRooks == 2) && (blackRooks == 2)) ||
				// rook vs rook + minor piece
				((whiteExtras == 2) && (blackExtras == 1) && (whiteRooks == 1) && (blackRooks == 1) && (whiteMinors == 1)) ||
				((whiteExtras == 1) && (blackExtras == 2) && (whiteRooks == 1) && (blackRooks == 1) && (blackMinors == 1)) ||
				// 2 rooks vs rook + minor
				((whiteExtras == 2) && (blackExtras == 2) && (whiteRooks == 2) && (blackRooks == 1) && (blackMinors == 1)) ||
				((whiteExtras == 2) && (blackExtras == 2) && (whiteRooks == 1) && (blackRooks == 2) && (whiteMinors == 1));
			if (drawish) score = score / 8;
		}
		
		if (potentiallyDrawishQueenful) {
			int whiteQueens = Popcount(board.WhiteQueenBits);
			int blackQueens = Popcount(board.BlackQueenBits);
			bool drawish = 
				// queen vs queen
				((whiteExtras == 1) && (blackExtras == 1) && (whiteQueens == 1) && (blackQueens == 1)) ||
				// queen vs 2 bishops
				((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackBishops == 2)) ||
				((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteBishops == 2)) ||
				// queen vs 2 knights
				((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackKnights == 2)) ||
				((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteKnights == 2)) ||
				// queen vs 2 rooks
				((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackRooks == 2)) ||
				((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteRooks == 2)) ||
				// queen vs rook + minor
				((whiteExtras == 1) && (blackExtras == 2) && (whiteQueens == 1) && (blackRooks == 1) && (blackMinors == 1)) ||
				((whiteExtras == 2) && (blackExtras == 1) && (blackQueens == 1) && (whiteRooks == 1) && (whiteMinors == 1));
			if (drawish) score = score / 8;
		}
	}

	// idea: taper eval close to 0 if the fifty-move counter is high

	return score;
}

inline static const int EvaluateBoard(Board& board, const int level) {
	return EvaluateBoard(board, level, Weights);
}
