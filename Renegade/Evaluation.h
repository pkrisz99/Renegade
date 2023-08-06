#pragma once
#include "Board.h"
#include "Move.h"

extern uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetQueenAttacks(const int square, const uint64_t occupancy);

struct EvaluationFeatures {

	// Weight size and its array
	static constexpr int WeightSize = 886;
	TaperedScore Weights[WeightSize];

	// King safety constants
	static constexpr int DangerMultipliers[8] = { 0, 50, 70, 80, 90, 95, 98, 100 };
	static constexpr int PieceDangers[7] = { 0, 1, 2, 2, 3, 5, 4 };

	// Indices (these are here to be easily used for tuning)
	constexpr int IndexPieceMaterial(const uint8_t pieceType) const { return pieceType - 1; }
	constexpr int IndexPSQT(const uint8_t pieceType, const uint8_t sq) const { return 6 + (pieceType - 1) * 64 + sq; }
	constexpr int IndexKnightMobility(const uint8_t mobility) const { return 390 + mobility; }
	constexpr int IndexBishopMobility(const uint8_t mobility) const { return 399 + mobility; }
	constexpr int IndexRookMobility(const uint8_t mobility) const { return 413 + mobility; }
	constexpr int IndexQueenMobility(const uint8_t mobility) const { return 428 + mobility; }
	constexpr int IndexKingDanger(const uint8_t danger) const { return 455 + danger; }
	//constexpr int IndexPassedPawn(const uint8_t rank) const { return 481 + rank; }
	constexpr int IndexBlockedPasser(const uint8_t rank) const { return 489 + rank; }
	constexpr int IndexIsolatedPawn(const uint8_t file) const { return 497 + file; }
	const int IndexDoubledPawns = 505;
	const int IndexTripledPawns = 506;
	const int IndexBishopPair = 507;
	//const int IndexRookOnOpenFile = 508;
	//const int IndexRookOnSemiOpenFile = 509;
	const int IndexKnightOutpost = 510;
	const int IndexTempoBonus = 513;
	constexpr int IndexPawnThreats(const uint8_t attackedPieceType) const { return 513 + attackedPieceType; };
	constexpr int IndexKnightThreats(const uint8_t attackedPieceType) const { return 519 + attackedPieceType; };
	constexpr int IndexBishopThreats(const uint8_t attackedPieceType) const { return 525 + attackedPieceType; };
	constexpr int IndexRookThreats(const uint8_t attackedPieceType) const { return 531 + attackedPieceType; };
	constexpr int IndexQueenThreats(const uint8_t attackedPieceType) const { return 537 + attackedPieceType; };
	constexpr int IndexKingThreats(const uint8_t attackedPieceType) const { return 543 + attackedPieceType; };
	constexpr int IndexPawnSupportingPawn(const uint8_t rank) const { return 550 + rank; };
	constexpr int IndexPawnPhalanx(const uint8_t rank) const { return 558 + rank; };
	constexpr int IndexRookOnOpenFile(const uint8_t sq) const { return 566 + sq; };
	constexpr int IndexRookOnSemiOpenFile(const uint8_t sq) const { return 630 + sq; };
	constexpr int IndexPassedPawn(const uint8_t sq) const { return 694 + sq; }
	constexpr int IndexKingOnOpenFile(const uint8_t sq) const { return 758 + sq; }
	constexpr int IndexKingOnSemiOpenFile(const uint8_t sq) const { return 822 + sq; }

	// Shorthand for retrieving the evaluation
	inline const TaperedScore& GetMaterial(const uint8_t pieceType) const { return Weights[IndexPieceMaterial(pieceType)]; }
	inline const TaperedScore& GetPSQT(const uint8_t pieceType, const uint8_t sq) const { return Weights[IndexPSQT(pieceType, sq)]; }
	inline const TaperedScore& GetKnightMobility(const uint8_t mobility) const { return Weights[IndexKnightMobility(mobility)]; }
	inline const TaperedScore& GetBishopMobility(const uint8_t mobility) const { return Weights[IndexBishopMobility(mobility)]; }
	inline const TaperedScore& GetRookMobility(const uint8_t mobility) const { return Weights[IndexRookMobility(mobility)]; }
	inline const TaperedScore& GetQueenMobility(const uint8_t mobility) const { return Weights[IndexQueenMobility(mobility)]; }
	inline const TaperedScore& GetKingDanger(const uint8_t danger) const { return Weights[IndexKingDanger(danger)]; }
	inline const TaperedScore& GetPassedPawnEval(const uint8_t sq) const { return Weights[IndexPassedPawn(sq)]; }
	inline const TaperedScore& GetBlockedPasserEval(const uint8_t rank) const { return Weights[IndexBlockedPasser(rank)]; }
	inline const TaperedScore& GetIsolatedPawnEval(const uint8_t file) const { return Weights[IndexIsolatedPawn(file)]; }
	inline const TaperedScore& GetDoubledPawnEval() const { return Weights[IndexDoubledPawns]; }
	inline const TaperedScore& GetTripledPawnEval() const { return Weights[IndexTripledPawns]; }
	inline const TaperedScore& GetBishopPairEval() const { return Weights[IndexBishopPair]; }
	//inline const TaperedScore& GetRookOnOpenFileEval() const { return Weights[IndexRookOnOpenFile]; }
	//inline const TaperedScore& GetRookOnSemiOpenFileEval() const { return Weights[IndexRookOnSemiOpenFile]; }
	inline const TaperedScore& GetKnightOutpostEval() const { return Weights[IndexKnightOutpost]; }
	inline const TaperedScore& GetTempoBonus() const { return Weights[IndexTempoBonus]; }
	inline const TaperedScore& GetPawnThreat(const uint8_t attackedPieceType) const { return Weights[IndexPawnThreats(attackedPieceType)]; }
	inline const TaperedScore& GetKnightThreat(const uint8_t attackedPieceType) const { return Weights[IndexKnightThreats(attackedPieceType)]; }
	inline const TaperedScore& GetBishopThreat(const uint8_t attackedPieceType) const { return Weights[IndexBishopThreats(attackedPieceType)]; }
	inline const TaperedScore& GetRookThreat(const uint8_t attackedPieceType) const { return Weights[IndexRookThreats(attackedPieceType)]; }
	inline const TaperedScore& GetQueenThreat(const uint8_t attackedPieceType) const { return Weights[IndexQueenThreats(attackedPieceType)]; }
	inline const TaperedScore& GetKingThreat(const uint8_t attackedPieceType) const { return Weights[IndexKingThreats(attackedPieceType)]; }
	inline const TaperedScore& GetPawnSupportingPawn(const uint8_t rank) const { return Weights[IndexPawnSupportingPawn(rank)]; }
	inline const TaperedScore& GetPawnPhalanx(const uint8_t rank) const { return Weights[IndexPawnPhalanx(rank)]; }
	inline const TaperedScore& GetRookOnOpenFileBonus(const uint8_t sq) const { return Weights[IndexRookOnOpenFile(sq)]; }
	inline const TaperedScore& GetRookOnSemiOpenFileBonus(const uint8_t sq) const { return Weights[IndexRookOnSemiOpenFile(sq)]; }
	inline const TaperedScore& GetKingOnOpenFileEval(const uint8_t sq) const { return Weights[IndexKingOnOpenFile(sq)]; }
	inline const TaperedScore& GetKingOnSemiOpenFileEval(const uint8_t sq) const { return Weights[IndexKingOnSemiOpenFile(sq)]; }
};


//typedef TaperedScore S; // using S as tapered score seems somewhat standard
#define S(early, late) TaperedScore{early, late}

constexpr EvaluationFeatures Weights = {

	// 1. Material values (pawn, knight, bishop, rook, queen, king)
	S(80, 92), S(356, 355), S(368, 380), S(479, 684), S(1107, 1245), S(0, 0), 

	// 2. Piece-square tables
	// Be careful, counter-intuitively the 1st element corresponds to white's bottom-left corner

	// 2.1 Pawn PSQT
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(-18, 29), S(-18, 40), S(-9, 30), S(-5, 32), S(3, 42), S(30, 23), S(38, 19), S(1, 8), S(-28, 21), S(-27, 29), S(-16, 16), S(-13, 24), S(0, 23), S(-2, 17), S(14, 14), S(-4, 3), S(-21, 25), S(-24, 39), S(-10, 19), S(2, 16), S(4, 16), S(8, 12), S(1, 22), S(-2, 7), S(-16, 41), S(-15, 41), S(-10, 27), S(-7, 14), S(15, 14), S(19, 10), S(11, 28), S(12, 16), S(-5, 64), S(-32, 85), S(6, 50), S(17, 45), S(21, 40), S(67, 29), S(46, 75), S(32, 59), S(80, 208), S(99, 200), S(60, 206), S(113, 164), S(61, 169), S(79, 162), S(-1, 212), S(2, 211), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
	
	// 2.2 Knight PSQT
	S(-82, -21), S(-40, -16), S(-41, -5), S(-23, -1), S(-20, 4), S(-14, -8), S(-35, -10), S(-58, -17), S(-49, -20), S(-39, 2), S(-21, 4), S(-5, 8), S(-5, 7), S(-8, 4), S(-19, -4), S(-22, 9), S(-42, -6), S(-19, 6), S(-4, 14), S(4, 28), S(18, 25), S(6, 8), S(2, 3), S(-16, -3), S(-21, 15), S(-6, 8), S(7, 27), S(14, 26), S(22, 30), S(19, 14), S(28, 5), S(-5, 8), S(-7, 5), S(7, 16), S(24, 25), S(48, 26), S(32, 19), S(50, 19), S(20, 13), S(29, -7), S(-17, -4), S(9, 3), S(39, 21), S(50, 12), S(59, 7), S(86, -7), S(30, -7), S(13, -18), S(-47, -11), S(-19, -4), S(9, -9), S(11, 2), S(5, -11), S(44, -18), S(11, -18), S(0, -35), S(-130, -84), S(-110, -22), S(-59, -3), S(-38, -15), S(-20, -8), S(-84, -30), S(-96, -19), S(-114, -83),

	// 2.3 Bishop PSQT
	S(-8, -17), S(20, -6), S(-4, -5), S(-10, -5), S(-3, -7), S(-10, 6), S(12, -16), S(12, -42), S(8, -13), S(7, -15), S(18, -20), S(-1, 0), S(11, -4), S(20, -11), S(27, -7), S(13, -26), S(-12, -7), S(13, 1), S(9, 5), S(10, 8), S(14, 14), S(12, 6), S(13, -4), S(10, -16), S(-12, -8), S(-13, 5), S(4, 10), S(28, 7), S(27, 4), S(4, 6), S(-4, 3), S(0, -20), S(-21, 2), S(-4, 6), S(8, 4), S(35, 14), S(27, 4), S(18, 7), S(-4, 4), S(-10, -5), S(-8, 3), S(14, -5), S(10, 5), S(21, -12), S(17, -6), S(59, -3), S(37, -8), S(23, -4), S(-26, -15), S(-4, -9), S(-14, -4), S(-17, -4), S(-4, -18), S(-7, -10), S(0, -15), S(-21, -17), S(-24, -2), S(-54, 6), S(-29, -10), S(-78, 3), S(-74, -3), S(-74, -8), S(-44, -8), S(-59, -16),

	// 2.4 Rook PSQT
	S(-18, 13), S(-12, 6), S(-5, 6), S(7, 0), S(10, -5), S(7, 3), S(-3, -14), S(-18, -10), S(-33, 3), S(-27, 14), S(-15, 5), S(-11, 2), S(-5, -8), S(3, -10), S(15, -17), S(-27, -18), S(-36, 13), S(-21, 18), S(-22, 12), S(-14, 7), S(-12, 1), S(-1, -5), S(29, -22), S(5, -27), S(-36, 22), S(-12, 30), S(-11, 20), S(-8, 14), S(-7, 6), S(-9, 9), S(11, 1), S(-12, -6), S(-19, 23), S(3, 17), S(-6, 30), S(2, 18), S(1, 3), S(19, 1), S(22, 6), S(12, -8), S(-25, 20), S(21, 22), S(13, 19), S(7, 13), S(40, -5), S(50, -4), S(94, -7), S(49, -12), S(-18, 18), S(-3, 33), S(12, 35), S(18, 23), S(5, 20), S(58, 14), S(44, 12), S(37, 1), S(-42, 21), S(4, 22), S(-5, 37), S(-19, 34), S(9, 34), S(14, 25), S(47, 20), S(35, 17),
	
	// 2.5 Queen PSQT
	S(-14, -19), S(-7, -16), S(0, -16), S(2, 6), S(4, -9), S(-7, -19), S(-1, -22), S(-10, -24), S(-9, -19), S(-4, -24), S(2, -16), S(7, -2), S(6, 3), S(14, -23), S(12, -35), S(30, -72), S(-16, -9), S(-5, 4), S(-5, 15), S(-10, 18), S(-2, 27), S(4, 21), S(12, 12), S(3, 12), S(-12, 3), S(-12, 13), S(-9, 17), S(-4, 36), S(-6, 35), S(-5, 38), S(6, 23), S(4, 39), S(-20, 22), S(-8, 15), S(-11, 22), S(-14, 39), S(1, 43), S(5, 50), S(16, 49), S(13, 31), S(-5, 4), S(-10, 13), S(-8, 40), S(-3, 48), S(14, 50), S(60, 43), S(72, 4), S(51, 36), S(-15, 4), S(-30, 16), S(-19, 34), S(-24, 54), S(-7, 58), S(13, 42), S(-5, 29), S(49, 23), S(-42, 17), S(-19, 10), S(0, 33), S(-14, 53), S(11, 37), S(31, 25), S(49, -13), S(-4, 18),

	// 2.6 King PSQT
	S(38, -99), S(75, -72), S(43, -44), S(-53, -23), S(3, -53), S(-31, -28), S(43, -64), S(44, -108), S(54, -52), S(23, -18), S(5, -7), S(-31, 9), S(-38, 14), S(-17, -1), S(26, -25), S(27, -47), S(-44, -27), S(6, -12), S(-40, 23), S(-56, 38), S(-48, 41), S(-60, 23), S(-29, -2), S(-70, -17), S(-57, -18), S(-39, 8), S(-59, 39), S(-77, 61), S(-81, 64), S(-43, 34), S(-74, 18), S(-148, 4), S(-65, -10), S(-24, 24), S(-30, 57), S(-79, 74), S(-55, 57), S(-52, 45), S(-44, 26), S(-128, 11), S(-98, -2), S(49, 24), S(-1, 66), S(9, 66), S(4, 55), S(27, 45), S(12, 33), S(-31, -4), S(-50, -9), S(44, 31), S(18, 56), S(32, 41), S(12, 33), S(12, 38), S(34, 31), S(34, -9), S(11, -98), S(36, -10), S(67, -5), S(-60, 13), S(-38, -13), S(29, -17), S(22, -3), S(79, -96),
	
	// 3. Piece mobility
	// Score tables depending on the pseudolegal moves available

	// 3.1 Knight mobility (0-8)
	S(-27, -36), S(-6, -4), S(6, 14), S(10, 24), S(15, 31), S(20, 38), S(26, 38), S(33, 32), S(35, 22),

	// 3.2 Bishop mobility (0-13)
	S(-27, -46), S(-15, -20), S(-3, -6), S(4, 8), S(12, 16), S(16, 28), S(20, 32), S(23, 35), S(22, 41), S(26, 36), S(32, 31), S(41, 29), S(39, 42), S(42, 24),

	// 3.3 Rook mobility (0-14)
	S(-20, -25), S(-8, -10), S(-4, -6), S(0, 4), S(0, 9), S(7, 12), S(9, 18), S(13, 21), S(17, 24), S(22, 28), S(25, 29), S(24, 34), S(32, 34), S(43, 29), S(45, 30),

	// 3.4 Queen mobility (0-27)
	S(-6, -97), S(-5, -123), S(-13, -94), S(-10, -69), S(-8, -55), S(-5, -42), S(-1, -26), S(1, -16), S(2, 0), S(6, -1), S(7, 13), S(10, 18), S(11, 25), S(12, 29), S(11, 38), S(14, 46), S(11, 60), S(9, 69), S(18, 69), S(36, 62), S(23, 81), S(72, 58), S(61, 70), S(72, 63), S(128, 70), S(100, 49), S(98, 71), S(92, 60),


	// 4. King safety (1-25 danger points)
	// Danger points are given for attacks near the king, and then scaled according to the attacker count

	S(-5, -5), S(-5, -5), S(-20, -20), S(-25, -25), S(-36, -36),
	S(-56, -56), S(-72, -72), S(-90, -90), S(-133, -133), S(-190, -190),
	S(-222, -222), S(-252, -252), S(-255, -255), S(-178, -178), S(-322, -322),
	S(-332, -332), S(-350, -350), S(-370, -370), S(-400, -400), S(-422, -422),
	S(-425, -425), S(-430, -430), S(-435, -435), S(-440, -440), S(-445, -445),

	// 5. Pawn structure
	// Collection of features to evaluate the pawn structure

	// 5.1 Passed pawns by rank -- unused
	S(0, 0), S(-6, 9), S(-11, 19), S(-8, 45), S(18, 74), S(46, 118), S(45, 78), S(0, 0),

	// 5.2 Blocked passed pawn penalties by rank
	S(0, 0), S(-25, 13), S(-14, 10), S(-15, -15), S(-6, -37), S(11, -110), S(-31, -126), S(0, 0),

	// 5.3 Isolated pawns by file
	S(-9, 2), S(-6, -12), S(-12, -7), S(-11, -15), S(-12, -13), S(-13, -6), S(-1, -14), S(-5, 0),

	// 5.4 Doubled and tripled pawns
	S(-6, -23), S(4, -61),

	// 6. Misc & piece-specific evaluation

	// 6.1 Bishop pairs
	S(23, 73),

	// 6.2 Rook on open and semi-open file -- unused
	S(29, 12), S(13, 9),

	// 6.3 Knight outposts
	S(7, 21),

	// 6.4 Pawn attacking minor and major pieces -- unused
	S(51, 28), S(56, 32),

	// 6.5 Tempo bonus
	S(20, 0),

	// 7. To be sorted

	// 7.1 Threat matrix
	// It somewhat helps to see beyond the horizon, also it ruins nps
	// Threatened king values are 0 due to the tuning dataset lacking such positions

	// Pawn attacking
	S(0, 0), S(63, -1), S(63, 34), S(80, -13), S(62, -19), S(0, 0),
	// Knight attacking
	S(-11, 4), S(0, 0), S(33, 19), S(62, -6), S(44, -40), S(0, 0),
	// Bishop attacking
	S(-4, 8), S(17, 23), S(0, 0), S(44, 3), S(64, 58), S(0, 0),
	// Rook attacking
	S(-7, 5), S(5, 9), S(15, 4), S(0, 0), S(65, 3), S(0, 0),
	// Queen attacking
	S(-6, 11), S(0, 4), S(-1, 22), S(-1, 5), S(0, 0), S(0, 0),
	// King attacking
	S(23, 35), S(-40, 17), S(-30, 28), S(-40, 11), S(0, 0), S(0, 0),

	// 7.2 Supported pawn bonus (by rank)
	S(0, 0), S(0, 0), S(18, 12), S(13, 7), S(16, 15), S(39, 35), S(150, 30), S(0, 0),

	// 7.3 Pawn phalanx (by rank)
	S(0, 0), S(7, -4), S(15, 7), S(26, 17), S(59, 54), S(122, 122), S(122, 122), S(0, 0),

	// 7.4 Rook open file bonus PSQT
	S(30, 8), S(24, 26), S(20, 32), S(19, 20), S(31, 16), S(43, 4), S(81, 18), S(93, 24), S(40, 5), S(23, 6), S(31, 16), S(24, 16), S(36, 18), S(34, 14), S(38, 9), S(77, 11), S(29, 9), S(11, 7), S(21, 10), S(13, 18), S(33, 17), S(31, 8), S(31, 14), S(71, 14), S(36, 1), S(0, 0), S(5, 12), S(12, 12), S(21, 15), S(24, 8), S(47, -6), S(85, 7), S(27, 4), S(21, 2), S(28, -1), S(21, 8), S(29, 10), S(24, -2), S(47, -8), S(71, 6), S(30, 4), S(9, -5), S(19, 3), S(25, 10), S(25, 13), S(14, 11), S(27, -2), S(42, 5), S(36, 5), S(17, 1), S(17, 2), S(37, 0), S(33, 8), S(-6, 6), S(8, 3), S(35, 8), S(65, 7), S(16, 10), S(9, 10), S(13, 12), S(1, 0), S(21, 4), S(-9, 4), S(35, -4),
	
	// 7.5 Rook semi-open file bonus PSQT
	S(15, 7), S(15, 1), S(16, 5), S(17, 2), S(15, 3), S(17, -7), S(51, -5), S(35, -1), S(21, -3), S(22, -18), S(18, -3), S(33, -21), S(23, -10), S(15, 1), S(27, -16), S(37, -9), S(17, 6), S(6, -13), S(17, -6), S(22, -16), S(17, -8), S(4, -2), S(8, -4), S(4, 17), S(16, 19), S(-14, -9), S(9, -5), S(14, 1), S(17, -1), S(-7, 6), S(11, 3), S(13, 10), S(5, 49), S(3, 28), S(9, 17), S(29, 16), S(31, -1), S(13, 9), S(17, 10), S(23, 33), S(30, 65), S(28, 31), S(20, 27), S(45, 4), S(39, 22), S(38, -7), S(21, 10), S(-11, 44), S(45, 67), S(55, 19), S(30, 24), S(24, 31), S(0, 12), S(20, 9), S(21, 8), S(57, 39), S(47, 65), S(33, 36), S(-15, 27), S(26, 19), S(-37, 21), S(-6, 1), S(6, 1), S(-43, 35),

	// 7.6 Passed pawn bonus PSQT
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(-9, 8), S(-3, 12), S(-15, 13), S(-13, 11), S(4, -7), S(2, 5), S(12, 12), S(5, 8), S(-3, 14), S(-16, 24), S(-21, 22), S(-19, 15), S(-18, 16), S(-12, 17), S(-19, 41), S(17, 11), S(6, 55), S(1, 48), S(-11, 39), S(-5, 33), S(-17, 39), S(-4, 42), S(-14, 60), S(-3, 51), S(29, 93), S(24, 93), S(31, 67), S(21, 65), S(3, 63), S(15, 72), S(-12, 93), S(-10, 94), S(53, 162), S(79, 152), S(47, 139), S(27, 117), S(26, 115), S(16, 134), S(-6, 136), S(-6, 150), S(57, 89), S(27, 88), S(43, 87), S(36, 82), S(45, 79), S(35, 87), S(49, 85), S(48, 82), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),

	// 7.7 King on open file delta PSQT

	S(-120, -11), S(-120, -14), S(-102, -4), S(-76, 11), S(-71, 28), S(-97, 10), S(-110, 25), S(-102, 28), S(-120, -25), S(-120, -4), S(-89, 2), S(-88, 4), S(-102, 10), S(-102, 12), S(-112, 17), S(-46, -2), S(-68, -25), S(-46, 0), S(-64, -6), S(-71, 1), S(-96, 1), S(-49, 2), S(-77, 12), S(-46, -3), S(-40, -32), S(-34, -13), S(-77, -4), S(-101, 1), S(-95, -1), S(-67, 5), S(-41, 2), S(14, -5), S(-24, -26), S(-35, -20), S(-91, -18), S(-93, -14), S(-42, -3), S(-47, 2), S(-30, 0), S(-19, -4), S(11, -32), S(-47, -31), S(-57, -45), S(-96, -25), S(-40, -14), S(-43, -12), S(-41, -8), S(-18, -6), S(-46, -58), S(-54, -54), S(-104, -49), S(-24, -35), S(-43, -10), S(-43, -3), S(-34, -24), S(27, -19), S(-23, -35), S(-35, -86), S(-44, -53), S(-53, -28), S(-44, -6), S(-35, -9), S(-41, -46), S(56, -61),
	
	// 7.8 King on semi-open file delta PSQT

	S(-54, 116), S(-102, 74), S(-49, 28), S(-20, 25), S(-16, 19), S(-34, 18), S(-33, 28), S(-49, 67), S(-59, 66), S(-53, 28), S(-40, 26), S(-25, 11), S(-22, 0), S(-28, 10), S(-34, 20), S(-29, 27), S(-44, 49), S(-10, 13), S(-31, 2), S(-21, -9), S(-20, -18), S(-33, 11), S(-5, 9), S(-12, 29), S(40, -2), S(-44, 2), S(-35, 8), S(10, -23), S(-38, -18), S(-47, 1), S(-38, 3), S(-24, 8), S(27, -12), S(-46, -20), S(-48, -18), S(-5, -42), S(-28, -28), S(-61, -4), S(-41, -14), S(-55, -5), S(-56, -4), S(-17, -29), S(-104, -42), S(-9, -59), S(-5, -58), S(-50, -34), S(-12, -25), S(-21, -12), S(-6, -15), S(42, -39), S(-74, -64), S(-55, -34), S(-25, -39), S(-26, -29), S(-18, -26), S(-17, -54), S(12, 55), S(-14, 28), S(56, -83), S(-69, 19), S(-76, -68), S(-43, -31), S(-7, 42), S(-15, 17),
};

inline int LinearTaper(const int earlyValue, const int lateValue, const float phase) {
	return static_cast<int>((1.f - phase) * earlyValue + phase * lateValue);
}

inline float CalculateGamePhase(const Board& board) {
	const int remainingPawns = Popcount(board.WhitePawnBits | board.BlackPawnBits);
	const int remainingKnights = Popcount(board.WhiteKnightBits | board.BlackKnightBits);
	const int remainingBishops = Popcount(board.WhiteBishopBits | board.BlackBishopBits);
	const int remainingRooks = Popcount(board.WhiteRookBits | board.BlackRookBits);
	const int remainingQueens = Popcount(board.WhiteQueenBits | board.BlackQueenBits);
	const int remainingScore = remainingPawns + remainingKnights * 10 + remainingBishops * 10 + remainingRooks * 20 + remainingQueens * 40;
	const float phase = (256 - remainingScore) / (256.f);
	return std::clamp(phase, 0.f, 1.f);
}


inline bool IsDrawishEndgame(const Board& board, const uint64_t whitePieces, const uint64_t blackPieces);

int EvaluateBoard(const Board& board, const EvaluationFeatures& weights);

inline int EvaluateBoard(const Board& board) {
	return EvaluateBoard(board, Weights);
}
