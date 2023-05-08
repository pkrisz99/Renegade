#pragma once
#include "Board.h"
#include "Move.h"

extern uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetQueenAttacks(const int square, const uint64_t occupancy);

struct EvaluationFeatures {

	// Weight size and its array
	static constexpr int WeightSize = 514;
	TaperedScore Weights[WeightSize];

	// King safety constants
	static constexpr int DangerMultipliers[8] = { 0, 50, 70, 80, 90, 95, 98, 100 };
	static constexpr int PieceDangers[7] = { 0, 1, 2, 2, 3, 5, 4 };

	// Indices (these are here to be easily used for tuning)
	constexpr int IndexPieceMaterial(const uint8_t pieceType) const { return pieceType - 1; }
	constexpr int IndexPSQT(const uint8_t pieceType, const uint8_t sq) const { return 6 + (pieceType-1) * 64 + sq; }
	constexpr int IndexKnightMobility(const uint8_t mobility) const { return 390 + mobility; }
	constexpr int IndexBishopMobility(const uint8_t mobility) const { return 399 + mobility; }
	constexpr int IndexRookMobility(const uint8_t mobility) const { return 413 + mobility; }
	constexpr int IndexQueenMobility(const uint8_t mobility) const { return 428 + mobility; }
	constexpr int IndexKingDanger(const uint8_t danger) const { return 456 + danger; }
	const int IndexPassedPawn(const uint8_t rank) const { return 481 + rank; }
	const int IndexBlockedPasser(const uint8_t rank) const { return 489 + rank; }
	const int IndexIsolatedPawn(const uint8_t file) const { return 497 + file; }
	const int IndexDoubledPawns = 505;
	const int IndexTripledPawns = 506;
	const int IndexBishopPair = 507;
	const int IndexRookOnOpenFile = 508;
	const int IndexRookOnSemiOpenFile = 509;
	const int IndexKnightOutpost = 510;
	const int IndexPawnAttackingMinor = 511;
	const int IndexPawnAttackingMajor = 512;
	const int IndexTempoBonus = 513;

	// Shorthand for retrieving the evaluation
	const TaperedScore& GetMaterial(const uint8_t pieceType) const { return Weights[IndexPieceMaterial(pieceType)]; }
	const TaperedScore& GetPSQT(const uint8_t pieceType, const uint8_t sq) const { return Weights[IndexPSQT(pieceType, sq)]; }
	const TaperedScore& GetKnightMobility(const uint8_t mobility) const { return Weights[IndexKnightMobility(mobility)]; }
	const TaperedScore& GetBishopMobility(const uint8_t mobility) const { return Weights[IndexBishopMobility(mobility)]; }
	const TaperedScore& GetRookMobility(const uint8_t mobility) const { return Weights[IndexRookMobility(mobility)]; }
	const TaperedScore& GetQueenMobility(const uint8_t mobility) const { return Weights[IndexQueenMobility(mobility)]; }
	const TaperedScore& GetKingDanger(const uint8_t danger) const { return Weights[IndexKingDanger(danger)]; }
	const TaperedScore& GetPassedPawnEval(const uint8_t rank) const { return Weights[IndexPassedPawn(rank)]; }
	const TaperedScore& GetBlockedPasserEval(const uint8_t rank) const { return Weights[IndexBlockedPasser(rank)]; }
	const TaperedScore& GetIsolatedPawnEval(const uint8_t file) const { return Weights[IndexIsolatedPawn(file)]; }
	const TaperedScore& GetDoubledPawnEval() const { return Weights[IndexDoubledPawns]; }
	const TaperedScore& GetTripledPawnEval() const { return Weights[IndexTripledPawns]; }
	const TaperedScore& GetBishopPairEval() const { return Weights[IndexBishopPair]; }
	const TaperedScore& GetRookOnOpenFileEval() const { return Weights[IndexRookOnOpenFile]; }
	const TaperedScore& GetRookOnSemiOpenFileEval() const { return Weights[IndexRookOnSemiOpenFile]; }
	const TaperedScore& GetKnightOutpostEval() const { return Weights[IndexKnightOutpost]; }
	const TaperedScore& GetPawnAttackingMinorEval() const { return Weights[IndexPawnAttackingMinor]; }
	const TaperedScore& GetPawnAttackingMajorEval() const { return Weights[IndexPawnAttackingMajor]; }
	const TaperedScore& GetTempoBonus() const { return Weights[IndexTempoBonus]; }
};


typedef TaperedScore S; // using S as tapered score seems somewhat standard

static EvaluationFeatures Weights = {

	// 1. Material values (pawn, knight, bishop, rook, queen, king)
	S(85, 80), S(325, 289), S(330, 318), S(444, 560), S(998, 1016), S(0, 0),

	// 2. Piece-square tables
	// Be careful, counter-intuitively the 1st element corresponds to white's bottom-left corner

	// 2.1 Pawn PSQT
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
	S(-19, 17), S(-18, 30), S(-15, 19), S(-20, 32), S(-6, 31), S(16, 21), S(27, 14), S(-7, 4),
	S(-19, 12), S(-15, 23), S(-9, 12), S(-12, 24), S(-2, 23), S(2, 18), S(23, 14), S(4, 0),
	S(-24, 23), S(-28, 29), S(-9, 13), S(8, 5), S(3, 7), S(8, 10), S(4, 16), S(-10, 6),
	S(-16, 49), S(-12, 50), S(-2, 31), S(5, 18), S(27, 12), S(24, 19), S(19, 35), S(4, 29),
	S(18, 115), S(11, 138), S(31, 110), S(46, 81), S(51, 67), S(77, 65), S(56, 114), S(32, 100),
	S(96, 178), S(105, 170), S(72, 179), S(106, 144), S(65, 154), S(68, 128), S(-21, 186), S(-21, 193),
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),

	// 2.2 Knight PSQT
	S(-80, -37), S(-40, -22), S(-33, -11), S(-27, -1), S(-20, -8), S(-19, -17), S(-43, -6), S(-52, -21),
	S(-45, -14), S(-35, -4), S(-22, 4), S(-10, 6), S(-10, 6), S(-1, 0), S(-21, -20), S(-20, -11),
	S(-36, -18), S(-17, 1), S(0, 5), S(13, 18), S(21, 21), S(2, 4), S(0, -1), S(-24, -15),
	S(-23, 7), S(-8, 12), S(11, 27), S(16, 20), S(23, 27), S(15, 17), S(16, 5), S(-9, -4),
	S(-5, -9), S(-1, 14), S(24, 27), S(50, 24), S(24, 21), S(52, 19), S(8, 13), S(19, -3),
	S(-15, 4), S(29, -1), S(59, 13), S(70, 10), S(75, 11), S(70, 1), S(50, -13), S(-7, -14),
	S(-51, -9), S(1, -6), S(29, -11), S(31, -4), S(25, -11), S(60, 2), S(31, -28), S(20, -19),
	S(-110, -88), S(-100, -38), S(-39, -17), S(-32, -23), S(-26, -8), S(-80, -26), S(-80, -19), S(-110, -89),

	// 2.3 Bishop PSQT
	S(-8, -17), S(14, 0), S(-7, 5), S(-2, -9), S(-3, -5), S(-12, 10), S(18, -12), S(0, -22),
	S(2, -13), S(5, -11), S(16, -16), S(-8, 4), S(5, -4), S(14, -3), S(17, -3), S(1, -10),
	S(-2, -5), S(4, -7), S(7, 1), S(7, -2), S(10, 6), S(8, 2), S(5, 0), S(10, -14),
	S(-18, -12), S(-15, 3), S(-4, 4), S(26, -3), S(17, 7), S(0, -4), S(0, -5), S(2, -32),
	S(-13, -12), S(0, -6), S(16, 3), S(34, 4), S(27, -4), S(26, -1), S(4, -4), S(-4, -13),
	S(-10, -3), S(19, -8), S(26, -11), S(21, -12), S(15, -18), S(45, -12), S(49, -12), S(21, -10),
	S(-21, -13), S(16, -11), S(2, -20), S(-7, -9), S(16, -30), S(9, -20), S(20, -9), S(-5, -25),
	S(-4, -4), S(-70, -2), S(-9, -14), S(-70, -9), S(-70, -19), S(-70, -16), S(-46, -11), S(-43, -18),

	// 2.4 Rook PSQT
	S(-16, 9), S(-13, 7), S(-6, 6), S(7, 4), S(6, -3), S(5, -1), S(4, -6), S(-17, -3),
	S(-35, 3), S(-18, -1), S(-17, 5), S(-13, 3), S(-11, -4), S(2, -12), S(9, -10), S(-22, -8),
	S(-25, 5), S(-25, 6), S(-23, 5), S(-17, 2), S(-8, -4), S(-15, -12), S(9, -17), S(-3, -19),
	S(-28, 6), S(-26, 8), S(-11, 13), S(-15, 14), S(-8, 2), S(-24, 1), S(-10, 3), S(-21, 2),
	S(-21, 16), S(3, 8), S(-10, 17), S(2, 12), S(1, -5), S(8, -6), S(8, 2), S(2, -6),
	S(-6, 14), S(14, 11), S(11, 7), S(4, 12), S(31, -6), S(33, -4), S(64, 2), S(34, 1),
	S(-9, 20), S(1, 21), S(4, 26), S(15, 12), S(-1, 11), S(36, 9), S(24, 4), S(16, 16),
	S(-18, 29), S(-4, 11), S(13, 21), S(-13, 20), S(-5, 18), S(6, 16), S(32, 8), S(28, 12),

	// 2.5 Queen PSQT
	S(-2, -11), S(-7, -12), S(0, -16), S(6, 10), S(4, -5), S(-3, -19), S(-5, -22), S(-6, -16),
	S(-7, -15), S(-4, -16), S(6, -12), S(11, -2), S(10, 3), S(18, -21), S(6, -19), S(26, -68),
	S(-12, -3), S(-5, 2), S(-1, 7), S(-6, 10), S(-6, 25), S(2, 5), S(0, 12), S(-3, 20),
	S(-12, -5), S(-10, -3), S(-13, 5), S(-12, 34), S(-12, 31), S(-9, 26), S(0, 7), S(-4, 31),
	S(-18, 14), S(-6, 11), S(-11, 10), S(-20, 35), S(1, 27), S(1, 42), S(4, 33), S(9, 15),
	S(-1, 6), S(-14, 21), S(0, 34), S(-15, 36), S(12, 34), S(44, 41), S(60, 0), S(35, 40),
	S(-17, 16), S(-18, 10), S(-3, 34), S(-8, 44), S(9, 56), S(13, 26), S(11, 19), S(51, 27),
	S(-34, 15), S(-19, -6), S(-8, 17), S(-22, 45), S(3, 21), S(19, 21), S(45, -3), S(8, 20),

	// 2.6 King PSQT
	S(33, -82), S(59, -60), S(32, -43), S(-40, -33), S(6, -44), S(-25, -32), S(42, -55), S(39, -86),
	S(51, -47), S(17, -23), S(4, -11), S(-27, -2), S(-27, 3), S(-18, -2), S(20, -20), S(30, -40),
	S(-18, -34), S(3, -11), S(-27, 9), S(-43, 24), S(-31, 23), S(-43, 18), S(-16, -2), S(-43, -17),
	S(-23, -34), S(-26, 2), S(-41, 29), S(-54, 43), S(-67, 46), S(-22, 30), S(-28, 11), S(-56, -7),
	S(-13, -17), S(-14, 15), S(-34, 33), S(-53, 42), S(-49, 45), S(-25, 43), S(-12, 26), S(-20, -4),
	S(-36, -10), S(0, 10), S(-30, 35), S(-30, 40), S(-23, 46), S(0, 53), S(0, 30), S(1, 2),
	S(-37, -25), S(-13, 4), S(-42, 10), S(-13, 16), S(-26, 22), S(-5, 27), S(0, 20), S(10, -7),
	S(-51, -84), S(-37, -46), S(-29, -28), S(-43, -6), S(-34, -23), S(-4, -11), S(0, 0), S(-29, -90),

	// 3. Piece mobility
	// Score tables depending on the pseudolegal moves available
	
	// 3.1 Knight mobility (0-8)
	S(-21, -21), S(-6, -6), S(2, 2), S(5, 5), S(9, 9), S(11, 11), S(11, 11), S(11, 11), S(10, 10),
	
	// 3.2 Bishop mobility (0-13)
	S(-45, -45), S(-34, -34), S(-22, -22), S(-16, -16), S(-7, -7), S(3, 3), S(9, 9),
	S(13, 13), S(16, 16), S(14, 14), S(14, 14), S(16, 16), S(16, 16), S(8, 8),
	
	// 3.3 Rook mobility (0-14)
	S(-29, -29), S(-16, -16), S(-12, -12), S(-6, -6), S(-4, -4), S(1, 1), S(4, 4), S(6, 6), 
	S(9, 9), S(13, 13), S(13, 13), S(14, 14), S(16, 16), S(17, 17), S(17, 17),
	
	// 3.4 Queen mobility (0-27)
	S(-20, -55), S(-11, -80), S(-31, -39), S(-21, -37), S(-19, -44), S(-14, -33), S(-10, -14),
	S(-9, -29), S(-5, -13), S(-2, -17), S(-2, 2), S(0, -4), S(3, 2), S(-1, 15),
	S(3, 20), S(4, 27), S(3, 22), S(2, 42), S(6, 47), S(16, 48), S(25, 51),
	S(25, 51), S(19, 54), S(33, 47), S(28, 50), S(37, 56), S(20, 46), S(78, 75),

	// 4. King safety (1-25 danger points)
	// Danger points are given for attacks near the king, and then scaled according to the attacker count

	S(-5, -5), S(-5, -5), S(-20, -20), S(-25, -25), S(-36, -36), 
	S(-56, -56), S(-72, -72), S(-90, -90), S(-133, -133), S(-190, -190),
	S(-222, -222), S(-252, -252), S(-255, -255), S(-178, -178), S(-322, -322),
	S(-332, -332), S(-350, -350), S(-370, -370), S(-400, -400), S(-422, -422),
	S(-425, -425), S(-430, -430), S(-435, -435), S(-440, -440), S(-445, -445),
	
	// 5. Pawn structure
	// Collection of features to evaluate the pawn structure
	
	// 5.1 Passed pawns by rank
	S(0, 0), S(-5, 13), S(-7, 18), S(4, 41), S(26, 54), S(24, 58), S(35, 54), S(0, 0),
	
	// 5.2 Blocked passed pawn penalties by rank
	S(0, 0), S(-28, -2), S(-21, -6), S(-21, -25), S(-12, -46), S(-31, -56), S(-40, -76), S(0, 0),
	
	// 5.3 Isolated pawns by file
	S(-13, 1), S(-12, -9), S(-13, -14), S(-17, -20), S(-18, -18), S(-20, -8), S(-6, -16), S(-5, -3),

	// 5.4 Doubled and tripled pawns
	S(-7, -20), S(-12, -37),
	
	// 6. Misc & piece-specific evaluation

	// 6.1 Bishop pairs
	S(34, 55),

	// 6.2 Rook on open and semi-open file
	S(27, 12), S(9, 9),

	// 6.3 Knight outposts
	S(1, 13),

	// 6.4 Pawn attacking minor and major pieces
	S(51, 28), S(56, 32),

	// 6.5 Tempo bonus
	S(18, 0),
};

// Interpolation functions ------------------------------------------------------------------------

static inline int LinearTaper(const int earlyValue, const int lateValue, const float phase) {
	return static_cast<int>((1.f - phase) * earlyValue + phase * lateValue);
}

// Game phase calculation -------------------------------------------------------------------------

static const float CalculateGamePhase(const Board& board) {
	const int remainingPawns = Popcount(board.WhitePawnBits | board.BlackPawnBits);
	const int remainingKnights = Popcount(board.WhiteKnightBits | board.BlackKnightBits);
	const int remainingBishops = Popcount(board.WhiteBishopBits | board.BlackBishopBits);
	const int remainingRooks = Popcount(board.WhiteRookBits | board.BlackRookBits);
	const int remainingQueens = Popcount(board.WhiteQueenBits | board.BlackQueenBits);
	const int remainingScore = remainingPawns + remainingKnights * 10 + remainingBishops * 10 + remainingRooks * 20 + remainingQueens * 40;
	const float phase = (256 - remainingScore) / (256.f);
	return std::clamp(phase, 0.f, 1.f);
}

// Drawish endgame detection ----------------------------------------------------------------------

inline static const bool IsDrawishEndgame(const Board& board, const uint64_t whitePieces, const uint64_t blackPieces) {
	// Drawish endgame detection
	// To avoid simplifying down to a non-winning endgame with a nominal material advantage
	// This list is not complete, and probably should be expanded even more (e.g. by including pawns)
	// Source: Chess Programming Wiki + https://www.madchess.net/2021/04/08/madchess-3-0-beta-4d22dec-endgame-eval-scaling/
	bool endgame = (Popcount(whitePieces) <= 3) && (Popcount(blackPieces) <= 3);
	if (!endgame) return false;

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
		if (drawish) return true;
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
		if (drawish) return true;
	}
	return false;
}

// Board evaluation -------------------------------------------------------------------------------

inline static const int EvaluateBoard(const Board& board, const int level, const EvaluationFeatures& weights) {

	// Renegade's definitions in evaluation:
	// - attacks: squares that a piece can perform a capture on (neglecting checks and friendly pieces)
	// - mobility: sqaures that a piece can move to (pseudolegal, for now at least)

	//int score = 0, earlyScore = 0, lateScore = 0;

	TaperedScore taperedScore(0, 0);

	const uint64_t occupancy = board.GetOccupancy();
	const uint64_t whitePieces = board.GetOccupancy(PieceColor::White);
	const uint64_t blackPieces = board.GetOccupancy(PieceColor::Black);
	const float phase = CalculateGamePhase(board);

	int mobilityScore = 0;
	uint64_t allOccupancy = occupancy;
	uint64_t whiteAttacks = 0, blackAttacks = 0;

	int whiteDangerScore = 0;
	int blackDangerScore = 0;
	int whiteDangerPieces = 0;
	int blackDangerPieces = 0;

	const int whiteKingSquare = 63 - Lzcount(board.WhiteKingBits);
	const int whiteKingFile = GetSquareFile(whiteKingSquare);
	const int whiteKingRank = GetSquareRank(whiteKingSquare);
	const uint64_t whiteKingZone = KingArea[whiteKingSquare];
	const int blackKingSquare = 63 - Lzcount(board.BlackKingBits);
	const int blackKingFile = GetSquareFile(blackKingSquare);
	const int blackKingRank = GetSquareRank(blackKingSquare);
	const uint64_t blackKingZone = KingArea[blackKingSquare];

	const uint64_t whiteMajorBits = board.WhiteRookBits | board.WhiteQueenBits;
	const uint64_t whiteMinorBits = board.WhiteKnightBits | board.WhiteBishopBits;
	const uint64_t blackMajorBits = board.BlackRookBits | board.BlackQueenBits;
	const uint64_t blackMinorBits = board.BlackKnightBits | board.BlackBishopBits;

	uint64_t piecesOnBoard = occupancy;
	while (piecesOnBoard != 0) {
		const int sq = Popsquare(piecesOnBoard);
		const int piece = board.GetPieceAt(sq);
		const int pieceType = TypeOfPiece(piece);
		const int pieceColor = ColorOfPiece(piece);
		const int file = GetSquareFile(sq);
		const int rank = GetSquareRank(sq);
		uint64_t mobility = 0, attacks = 0;

		// Material and piece-square tables
		if (pieceColor == PieceColor::White) {
			taperedScore += weights.GetMaterial(pieceType);
			taperedScore += weights.GetPSQT(pieceType, sq);
		}
		else {
			taperedScore -= weights.GetMaterial(pieceType);
			taperedScore -= weights.GetPSQT(pieceType, Mirror[sq]);
		}

		// Piece-specific evaluation
		switch (piece) {

		case Piece::WhitePawn:
			// Get attacks & update king danger scores
			attacks = WhitePawnAttacks[sq] & ~whitePieces;
			whiteAttacks |= attacks;
			if ((blackKingZone & attacks) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Pawn];
				blackDangerPieces += 1;
			}
			// Isolated pawn evaluation
			if ((board.WhitePawnBits & IsolatedPawnMask[file]) == 0) {
				taperedScore += weights.GetIsolatedPawnEval(file);
			}
			// Passed pawn evaluation
			if (((WhitePassedPawnMask[sq] & board.BlackPawnBits) == 0) && ((WhitePassedPawnFilter[sq] & board.WhitePawnBits) == 0)) {
				taperedScore += weights.GetPassedPawnEval(rank);
				if (SquareBits[sq + 8] & blackPieces) taperedScore += weights.GetBlockedPasserEval(rank);
			}
			// Pawn attacking opponent's major or minor pieces
			if (((GetSquareFile(sq) != 0) && CheckBit(blackMajorBits, sq + 7)) || ((GetSquareFile(sq) != 7) && CheckBit(blackMajorBits, sq + 9))) {
				taperedScore += weights.GetPawnAttackingMajorEval();
			}
			else if (((GetSquareFile(sq) != 0) && CheckBit(blackMinorBits, sq + 7)) || ((GetSquareFile(sq) != 7) && CheckBit(blackMinorBits, sq + 9))) {
				taperedScore += weights.GetPawnAttackingMinorEval();
			}
			break;

		case Piece::BlackPawn:
			// Get attacks & update king danger scores
			attacks = BlackPawnAttacks[sq] & ~blackPieces;
			blackAttacks |= attacks;
			if ((whiteKingZone & attacks) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Pawn];
				whiteDangerPieces += 1;
			}
			// Isolated pawn evaluation
			if ((board.BlackPawnBits & IsolatedPawnMask[file]) == 0) {
				taperedScore -= weights.GetIsolatedPawnEval(7 - file);
			}
			// Passed pawn evaluation
			if (((BlackPassedPawnMask[sq] & board.WhitePawnBits) == 0) && ((BlackPassedPawnFilter[sq] & board.BlackPawnBits) == 0)) {
				taperedScore -= weights.GetPassedPawnEval(7 - rank);
				if (SquareBits[sq - 8] & whitePieces) taperedScore -= weights.GetBlockedPasserEval(7 - rank);
			}
			// Pawn attacking opponent's major or minor pieces
			if (((GetSquareFile(sq) != 0) && CheckBit(whiteMajorBits, sq - 9)) || ((GetSquareFile(sq) != 7) && CheckBit(whiteMajorBits, sq - 7))) {
				taperedScore -= weights.GetPawnAttackingMajorEval();
			}
			else if (((GetSquareFile(sq) != 0) && CheckBit(whiteMinorBits, sq - 9)) || ((GetSquareFile(sq) != 7) && CheckBit(whiteMinorBits, sq - 7))) {
				taperedScore -= weights.GetPawnAttackingMinorEval();
			}
			break;

		case Piece::WhiteKnight:
			// Get attacks, mobility & update king danger scores
			mobility = KnightMoveBits[sq] & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetKnightMobility(Popcount(mobility));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Knight];
				blackDangerPieces += 1;
			}
			// Knight outposts
			if (OutpostFilter[sq]) {
				if ((board.GetPieceAt(sq - 7) == Piece::WhitePawn) || (board.GetPieceAt(sq - 9) == Piece::WhitePawn)) {
					taperedScore += weights.GetKnightOutpostEval();
				}
			}

			break;

		case Piece::BlackKnight:
			// Get attacks, mobility & update king danger scores
			mobility = KnightMoveBits[sq] & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetKnightMobility(Popcount(mobility));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Knight];
				whiteDangerPieces += 1;
			}
			// Knight outposts
			if (OutpostFilter[sq]) {
				if ((board.GetPieceAt(sq + 7) == Piece::BlackPawn) || (board.GetPieceAt(sq + 9) == Piece::BlackPawn)) {
					taperedScore -= weights.GetKnightOutpostEval();
				}
			}
			break;

		case Piece::WhiteBishop:
			// Get attacks, mobility & update king danger scores
			mobility = GetBishopAttacks(sq, occupancy) & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetBishopMobility(Popcount(mobility));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Bishop];
				blackDangerPieces += 1;
			}
			break;

		case Piece::BlackBishop:
			// Get attacks, mobility & update king danger scores
			mobility = GetBishopAttacks(sq, occupancy) & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetBishopMobility(Popcount(mobility));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Bishop];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteRook:
			// Get attacks, mobility & update king danger scores
			mobility = GetRookAttacks(sq, occupancy) & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetRookMobility(Popcount(mobility));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Rook];
				blackDangerPieces += 1;
			}
			// Rook on open or semi-open file
			if ((board.WhitePawnBits & Files[file]) == 0) {
				if ((board.BlackPawnBits & Files[file]) == 0) { // open file
					taperedScore += weights.GetRookOnOpenFileEval();
				}
				else { // semi-open file
					taperedScore += weights.GetRookOnSemiOpenFileEval();
				}
			}
			break;

		case Piece::BlackRook:
			// Get attacks, mobility & update king danger scores
			mobility = GetRookAttacks(sq, occupancy) & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetRookMobility(Popcount(mobility));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Rook];
				whiteDangerPieces += 1;
			}
			// Rook on open or semi-open file
			if ((board.BlackPawnBits & Files[file]) == 0) {
				if ((board.WhitePawnBits & Files[file]) == 0) { // open file
					taperedScore -= weights.GetRookOnOpenFileEval();
				}
				else { // semi-open file
					taperedScore -= weights.GetRookOnSemiOpenFileEval();
				}
			}
			break;

		case Piece::WhiteQueen:
			// Get attacks, mobility & update king danger scores
			mobility = GetQueenAttacks(sq, occupancy) & ~whitePieces;
			whiteAttacks |= mobility;
			taperedScore += weights.GetQueenMobility(Popcount(mobility));
			if ((blackKingZone & mobility) != 0) {
				blackDangerScore += weights.PieceDangers[PieceType::Queen];
				blackDangerPieces += 1;
			}
			break;

		case Piece::BlackQueen:
			// Get attacks, mobility & update king danger scores
			mobility = GetQueenAttacks(sq, occupancy) & ~blackPieces;
			blackAttacks |= mobility;
			taperedScore -= weights.GetQueenMobility(Popcount(mobility));
			if ((whiteKingZone & mobility) != 0) {
				whiteDangerScore += weights.PieceDangers[PieceType::Queen];
				whiteDangerPieces += 1;
			}
			break;

		case Piece::WhiteKing:
			break;
		case Piece::BlackKing:
			break;
		}
	}

	// King safety
	const int whiteKingSafetyFinal = whiteDangerScore * weights.DangerMultipliers[std::min(whiteDangerPieces, 7)] / 100;
	const int blackKingSafetyFinal = blackDangerScore * weights.DangerMultipliers[std::min(blackDangerPieces, 7)] / 100;
	if (whiteKingSafetyFinal != 0) taperedScore += weights.GetKingDanger(std::min(whiteKingSafetyFinal, 25));
	if (blackKingSafetyFinal != 0) taperedScore -= weights.GetKingDanger(std::min(blackKingSafetyFinal, 25));

	// Bishop pair bonus
	if (Popcount(board.WhiteBishopBits) >= 2) taperedScore += weights.GetBishopPairEval();
	if (Popcount(board.BlackBishopBits) >= 2) taperedScore -= weights.GetBishopPairEval();

	// Doubled & tripled pawn penalties
	for (int i = 0; i < 8; i++) {
		const int whitePawnsOnFile = Popcount(board.WhitePawnBits & Files[i]);
		const int blackPawnsOnFile = Popcount(board.BlackPawnBits & Files[i]);
		if (whitePawnsOnFile == 2) taperedScore += weights.GetDoubledPawnEval();
		else if (whitePawnsOnFile > 2) taperedScore += weights.GetTripledPawnEval();
		if (blackPawnsOnFile == 2) taperedScore -= weights.GetDoubledPawnEval();
		else if (blackPawnsOnFile > 2) taperedScore -= weights.GetTripledPawnEval();
	}

	// Get untapered score
	int score = LinearTaper(taperedScore.early, taperedScore.late, phase);

	// Convert to the correct perspective
	if (!board.Turn) score *= -1;

	// Tempo bonus
	score += LinearTaper(weights.GetTempoBonus().early, weights.GetTempoBonus().late, phase);

	// Drawish endgame
	if (IsDrawishEndgame(board, whitePieces, blackPieces)) score /= 8;

	return score;
}

inline static const int EvaluateBoard(Board& board, const int level) {
	return EvaluateBoard(board, level, Weights);
}
