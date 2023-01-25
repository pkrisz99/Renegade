#pragma once
#include "Board.h"
#include "Move.h"

static const int MateEval = 1000000;
static const int NoEval = -666666666;
static const int NegativeInfinity = -333333333; // Inventing a new kind of math here
static const int PositiveInfinity = 444444444; // These numbers are easy to recognize if something goes wrong

const static int WeightsSize = 793;

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
	-30, -30,   0,   0,   0,   0, -30, -30,
	-30, -10,  20,  30,  30,  20, -10, -30,
	-30, -10,  30,  40,  40,  30, -10, -30,
	-30, -10,  30,  40,  40,  30, -10, -30,
	-30, -10,  20,  30,  30,  20, -10, -30,
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

	// 15. Bishop pair bonus (early & late game)
	30,
	40,

	// 16. Tempo bonus (early & late game)
	18,
	0,

	// 17. Doubled and tripled pawn penalty (early doubled, late doubled, early tripled, late tripled)
	-15,
	-20,
	-30,
	-40,

	// 18. Passed pawn bonus (early & late game)
	5,
	25,

	// 19. Defended pawn bonus (early & late game)
	0,
	0,

	// 20. King safety weight
	-84,

	/*
	// 21. Pawn mobility (early & late game)
	2, 10, // added if pawn can move

	// 22. Knight mobility
	25

	// 23. Bishop mobility...
	30,
	
	// 24. Rook mobility....
	
	// 25. Queen mobility...

	// 26. King mobility...*/


	// +: outposts, isolated pawns, open files...
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

static const int LinearTaper(const int earlyValue, const int lateValue, const float phase) {
	return (int)((1 - phase) * (float)earlyValue + phase * (float)lateValue);
}

static const int SigmoidishTaper(const int lateValue, const float phase) {
	return (int) (( 1.f / (1.f + std::exp(-6*phase+3))) * lateValue);
}

static const float CalculateGamePhase(Board &board) {
	int remainingPieces = Popcount(board.GetOccupancy());
	float phase = (32.f - remainingPieces) / (32.f - 4.f);
	return std::clamp(phase, 0.f, 1.f);
}

static const std::tuple<int, uint64_t> KnightMobility(int square, uint64_t friendlyPieces) {
	uint64_t mobility = KnightMoveBits[square] & ~friendlyPieces;
	float mobilityPhase = Popcount(mobility) / 8.f;
	return  { LinearTaper(-22, 22, mobilityPhase), mobility };
}

static const std::tuple<int, uint64_t> RookMobility(Board& board, int square, uint64_t friendlyPieces, uint64_t opponentPieces) {
	uint64_t verticalMobility = (board.GenerateSlidingAttacksShiftDown(8, ~Bitboards::Rank1, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(8, ~Bitboards::Rank8, SquareBits[square], friendlyPieces, opponentPieces))
		& ~SquareBits[square];
	uint64_t horizontalMobility = (board.GenerateSlidingAttacksShiftDown(1, ~Bitboards::FileA, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(1, ~Bitboards::FileH, SquareBits[square], friendlyPieces, opponentPieces))
		& ~SquareBits[square];
	float horizontalMobilityPhase = Popcount(horizontalMobility) / 7.f;
	float verticalMobilityPhase = Popcount(verticalMobility) / 7.f;
	return  { LinearTaper(-16, 16, horizontalMobilityPhase) + LinearTaper(-20, 20, verticalMobilityPhase), verticalMobility | horizontalMobility };
}

static const std::tuple<int, uint64_t> BishopMobility(Board& board, int square, uint64_t friendlyPieces, uint64_t opponentPieces) {
	uint64_t mobility = (board.GenerateSlidingAttacksShiftDown(7, ~Bitboards::FileH & ~Bitboards::Rank1, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftDown(9, ~Bitboards::FileA & ~Bitboards::Rank1, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(9, ~Bitboards::FileH & ~Bitboards::Rank8, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(7, ~Bitboards::FileA & ~Bitboards::Rank8, SquareBits[square], friendlyPieces, opponentPieces))
		& ~SquareBits[square];
	float mobilityPhase = Popcount(mobility) / 13.f;
	return { LinearTaper(-16, 16, mobilityPhase), mobility };
}

static const std::tuple<int, uint64_t> QueenMobility(Board& board, int square, uint64_t friendlyPieces, uint64_t opponentPieces) {
	uint64_t mobility = (board.GenerateSlidingAttacksShiftDown(8, ~Bitboards::Rank1, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(8, ~Bitboards::Rank8, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftDown(1, ~Bitboards::FileA, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(1, ~Bitboards::FileH, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftDown(7, ~Bitboards::FileH & ~Bitboards::Rank1, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftDown(9, ~Bitboards::FileA & ~Bitboards::Rank1, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(9, ~Bitboards::FileH & ~Bitboards::Rank8, SquareBits[square], friendlyPieces, opponentPieces)
		| board.GenerateSlidingAttacksShiftUp(7, ~Bitboards::FileA & ~Bitboards::Rank8, SquareBits[square], friendlyPieces, opponentPieces))
		& ~SquareBits[square];
	float mobilityPhase = Popcount(mobility) / 27.f;
	return { LinearTaper(-32, 32, mobilityPhase), mobility };
}


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

const int IndexDefendedPawnEarly = 790;
const int IndexDefendedPawnLate = 791;
const int IndexKingSafety = 792;


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
	uint64_t whitePieces = board.GetOccupancy(PieceColor::White);
	uint64_t blackPieces = board.GetOccupancy(PieceColor::Black);
	float phase = CalculateGamePhase(board);
	//uint64_t ourAttacks = board.CalculateAttackedSquares(board.Turn); // Opponent attacks are stored in the board
	//uint64_t whiteAttacks = board.Turn ? ourAttacks : board.AttackedSquares;
	//uint64_t blackAttacks = board.Turn ? board.AttackedSquares : ourAttacks;

	int mobilityScore = 0;
	uint64_t whiteAttacks = 0;
	uint64_t blackAttacks = 0;
	uint64_t allOccupancy = occupancy;
	std::tuple<int, uint64_t> mob;

	while (occupancy != 0) {
		int i = 63 - Lzcount(occupancy);
		SetBitFalse(occupancy, i);
		int piece = board.GetPieceAt(i);
		int pieceType = TypeOfPiece(piece);
		int pieceColor = ColorOfPiece(piece);

		if (pieceColor == PieceColor::White) {
			score += LinearTaper(weights[IndexEarlyPSQT(pieceType, i)], weights[IndexLatePSQT(pieceType, i)], phase);
			score += LinearTaper(weights[IndexPieceValueEarly(pieceType)], weights[IndexPieceValueLate(pieceType)], phase);
		}
		else if (pieceColor == PieceColor::Black) {
			score -= LinearTaper(weights[IndexEarlyPSQT(pieceType, Mirror[i])], weights[IndexLatePSQT(pieceType, Mirror[i])], phase);
			score -= LinearTaper(weights[IndexPieceValueEarly(pieceType)], weights[IndexPieceValueLate(pieceType)], phase);
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

		// Mobility - todo: make this prettier
		switch (piece) {
		case Piece::WhitePawn:
			whiteAttacks |= WhitePawnAttacks[i] & ~whitePieces;
			break;
		case Piece::BlackPawn:
			blackAttacks |= BlackPawnAttacks[i] & ~blackPieces;
			break;

		case Piece::WhiteKnight:
			mob = KnightMobility(i, whitePieces);
			mobilityScore += get<0>(mob);
			whiteAttacks |= get<1>(mob);
			break;
		case Piece::BlackKnight:
			mob = KnightMobility(i, blackPieces);
			mobilityScore -= get<0>(mob);
			blackAttacks |= get<1>(mob);
			break;

		case Piece::WhiteBishop:
			mob = BishopMobility(board, i, whitePieces, blackPieces);
			mobilityScore += get<0>(mob);
			whiteAttacks |= get<1>(mob);
			break;
		case Piece::BlackBishop:
			mob = BishopMobility(board, i, blackPieces, whitePieces);
			mobilityScore -= get<0>(mob);
			blackAttacks |= get<1>(mob);
			break;

		case Piece::WhiteRook:
			mob = RookMobility(board, i, whitePieces, blackPieces);
			mobilityScore += get<0>(mob);
			whiteAttacks |= get<1>(mob);
			break;
		case Piece::BlackRook:
			mob = RookMobility(board, i, blackPieces, whitePieces);
			mobilityScore -= get<0>(mob);
			blackAttacks |= get<1>(mob);
			break;

		case Piece::WhiteQueen:
			mob = QueenMobility(board, i, whitePieces, blackPieces);
			mobilityScore += get<0>(mob);
			whiteAttacks |= get<1>(mob);
			break;
		case Piece::BlackQueen:
			mob = QueenMobility(board, i, blackPieces, whitePieces);
			mobilityScore -= get<0>(mob);
			blackAttacks |= get<1>(mob);
			break;
		
		case Piece::WhiteKing:
			whiteAttacks |= (KingMoveBits[i] & ~whitePieces);
			break;
		case Piece::BlackKing:
			blackAttacks |= (KingMoveBits[i] & ~blackPieces);
			break;
		}
	}

	// Taper mobility contribution to eval score
	// To avoid putting the queen out too early
	//score += LinearTaper(0, mobilityScore, std::min(phase * 7.f, 1.f));

	// Very rudimentary king safety
	whiteAttacks &= ~whitePieces;
	blackAttacks &= ~blackPieces;

	int whiteKingSquare = 63 - Lzcount(board.WhiteKingBits);
	const float whiteKingSafety = Popcount((SquareBits[whiteKingSquare] | KingMoveBits[whiteKingSquare]) & blackAttacks) / (Popcount(KingMoveBits[whiteKingSquare]) + 1.f);
	score += SigmoidishTaper(weights[IndexKingSafety], whiteKingSafety);

	int blackKingSquare = 63 - Lzcount(board.BlackKingBits);
	const float blackKingSafety = Popcount((SquareBits[blackKingSquare] | KingMoveBits[blackKingSquare]) & whiteAttacks) / (Popcount(KingMoveBits[blackKingSquare]) + 1.f);
	score -= SigmoidishTaper(weights[IndexKingSafety], blackKingSafety);
	

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

	// Attacked square count
	//const int controlWeight = 2;
	//score += (Popcount(whiteAttacks) - Popcount(blackAttacks)) * controlWeight;

	if (!board.Turn) score *= -1;

	// Tempo bonus
	score += LinearTaper(weights[IndexTempoEarly], weights[IndexTempoLate], phase);

	return score;
}

inline static const int EvaluateBoard(Board& board, const int level) {
	return EvaluateBoard(board, level, Weights);
}
