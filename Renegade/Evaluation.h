#pragma once
#include "Board.h"
#include "Move.h"

extern uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetQueenAttacks(const int square, const uint64_t occupancy);

struct EvaluationFeatures {

	// Weight size and its array
	static constexpr int WeightSize = 566;
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
	constexpr int IndexPassedPawn(const uint8_t rank) const { return 481 + rank; }
	constexpr int IndexBlockedPasser(const uint8_t rank) const { return 489 + rank; }
	constexpr int IndexIsolatedPawn(const uint8_t file) const { return 497 + file; }
	const int IndexDoubledPawns = 505;
	const int IndexTripledPawns = 506;
	const int IndexBishopPair = 507;
	const int IndexRookOnOpenFile = 508;
	const int IndexRookOnSemiOpenFile = 509;
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

	// Shorthand for retrieving the evaluation
	inline const TaperedScore& GetMaterial(const uint8_t pieceType) const { return Weights[IndexPieceMaterial(pieceType)]; }
	inline const TaperedScore& GetPSQT(const uint8_t pieceType, const uint8_t sq) const { return Weights[IndexPSQT(pieceType, sq)]; }
	inline const TaperedScore& GetKnightMobility(const uint8_t mobility) const { return Weights[IndexKnightMobility(mobility)]; }
	inline const TaperedScore& GetBishopMobility(const uint8_t mobility) const { return Weights[IndexBishopMobility(mobility)]; }
	inline const TaperedScore& GetRookMobility(const uint8_t mobility) const { return Weights[IndexRookMobility(mobility)]; }
	inline const TaperedScore& GetQueenMobility(const uint8_t mobility) const { return Weights[IndexQueenMobility(mobility)]; }
	inline const TaperedScore& GetKingDanger(const uint8_t danger) const { return Weights[IndexKingDanger(danger)]; }
	inline const TaperedScore& GetPassedPawnEval(const uint8_t rank) const { return Weights[IndexPassedPawn(rank)]; }
	inline const TaperedScore& GetBlockedPasserEval(const uint8_t rank) const { return Weights[IndexBlockedPasser(rank)]; }
	inline const TaperedScore& GetIsolatedPawnEval(const uint8_t file) const { return Weights[IndexIsolatedPawn(file)]; }
	inline const TaperedScore& GetDoubledPawnEval() const { return Weights[IndexDoubledPawns]; }
	inline const TaperedScore& GetTripledPawnEval() const { return Weights[IndexTripledPawns]; }
	inline const TaperedScore& GetBishopPairEval() const { return Weights[IndexBishopPair]; }
	inline const TaperedScore& GetRookOnOpenFileEval() const { return Weights[IndexRookOnOpenFile]; }
	inline const TaperedScore& GetRookOnSemiOpenFileEval() const { return Weights[IndexRookOnSemiOpenFile]; }
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
};


//typedef TaperedScore S; // using S as tapered score seems somewhat standard
#define S(early, late) TaperedScore{early, late}

constexpr EvaluationFeatures Weights = {

	// 1. Material values (pawn, knight, bishop, rook, queen, king)
	S(85, 80), S(325, 289), S(330, 318), S(444, 560), S(998, 1016), S(0, 0),

	// 2. Piece-square tables
	// Be careful, counter-intuitively the 1st element corresponds to white's bottom-left corner

	// 2.1 Pawn PSQT
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),
	S(-19, 24), S(-22, 32), S(-14, 25), S(-18, 30), S(-3, 34), S(18, 21), S(26, 14), S(-6, 9),
	S(-22, 18), S(-22, 25), S(-13, 15), S(-12, 21), S(-1, 20), S(-3, 19), S(14, 14), S(-3, 6),
	S(-21, 23), S(-23, 33), S(-12, 14), S(3, 9), S(1, 11), S(4, 10), S(-5, 22), S(-5, 6),
	S(-17, 48), S(-14, 47), S(-5, 31), S(-2, 17), S(18, 12), S(22, 13), S(13, 33), S(7, 25),
	S(6, 112), S(-1, 129), S(26, 98), S(37, 75), S(39, 65), S(71, 59), S(44, 102), S(20, 91),
	S(96, 184), S(93, 175), S(69, 191), S(110, 144), S(62, 155), S(80, 140), S(-9, 195), S(-9, 196),
	S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0), S(0, 0),

	// 2.2 Knight PSQT
	S(-80, -29), S(-40, -14), S(-41, -3), S(-23, 1), S(-22, 0), S(-15, -9), S(-39, -2), S(-56, -13),
	S(-49, -14), S(-39, 0), S(-22, 4), S(-6, 6), S(-6, 6), S(-9, 2), S(-21, -12), S(-22, -3),
	S(-40, -10), S(-21, 5), S(-4, 13), S(5, 20), S(17, 21), S(4, 8), S(0, 3), S(-16, -7),
	S(-21, 7), S(-8, 12), S(11, 25), S(16, 24), S(23, 31), S(19, 13), S(20, 9), S(-9, 0),
	S(-9, -1), S(3, 18), S(24, 27), S(50, 24), S(32, 21), S(54, 19), S(16, 11), S(23, -7),
	S(-23, -4), S(21, 1), S(51, 9), S(62, 6), S(67, 3), S(78, -3), S(42, -13), S(1, -14),
	S(-47, -7), S(-7, -10), S(21, -11), S(23, -2), S(17, -15), S(52, -6), S(23, -24), S(12, -27),
	S(-118, -80), S(-98, -30), S(-47, -9), S(-32, -15), S(-22, -4), S(-72, -18), S(-88, -15), S(-102, -81),

	// 2.3 Bishop PSQT
	S(-8, -15), S(16, -4), S(-5, -3), S(-10, -1), S(-7, -3), S(-12, 10), S(10, -12), S(8, -30),
	S(4, -9), S(7, -15), S(18, -16), S(0, 0), S(7, 0), S(16, -7), S(25, -5), S(9, -18),
	S(-10, -5), S(12, 1), S(11, 3), S(11, 6), S(14, 14), S(12, 4), S(9, 0), S(10, -14),
	S(-14, -4), S(-11, 3), S(4, 8), S(30, 5), S(25, 3), S(4, 4), S(-4, 3), S(2, -24),
	S(-13, -4), S(0, 2), S(12, 3), S(36, 8), S(27, 0), S(22, 3), S(-4, 4), S(-8, -9),
	S(-2, 1), S(15, -6), S(18, -3), S(21, -12), S(19, -10), S(53, -4), S(41, -12), S(25, -6),
	S(-25, -13), S(8, -15), S(-6, -12), S(-11, -5), S(8, -22), S(1, -16), S(12, -13), S(-13, -17),
	S(-12, -2), S(-62, 6), S(-17, -6), S(-66, -1), S(-62, -11), S(-62, -8), S(-42, -7), S(-47, -10),

	// 2.4 Rook PSQT
	S(-18, 11), S(-13, 9), S(-6, 10), S(7, 0), S(10, -3), S(7, 3), S(12, -6), S(-17, 1),
	S(-33, 3), S(-22, 7), S(-15, 7), S(-11, 3), S(-3, -4), S(4, -12), S(11, -14), S(-20, -8),
	S(-29, 9), S(-29, 8), S(-21, 7), S(-15, 4), S(-8, -2), S(-7, -4), S(17, -17), S(5, -19),
	S(-24, 14), S(-22, 16), S(-19, 15), S(-15, 14), S(-8, 10), S(-20, 9), S(-2, 3), S(-13, 2),
	S(-13, 24), S(1, 16), S(-6, 25), S(2, 16), S(9, 3), S(16, 2), S(16, 6), S(10, 2),
	S(-10, 18), S(18, 15), S(15, 15), S(6, 14), S(39, 2), S(41, 0), S(72, -2), S(42, -3),
	S(-13, 22), S(-7, 29), S(0, 30), S(19, 20), S(7, 19), S(32, 11), S(32, 12), S(24, 12),
	S(-16, 29), S(4, 19), S(5, 29), S(-5, 28), S(-7, 26), S(14, 24), S(40, 16), S(32, 16),

	// 2.5 Queen PSQT
	S(-10, -19), S(-9, -16), S(0, -16), S(2, 6), S(4, -5), S(-7, -19), S(3, -14), S(-14, -20),
	S(-11, -11), S(-4, -16), S(2, -12), S(7, -2), S(6, 3), S(14, -21), S(10, -27), S(30, -64),
	S(-12, -5), S(-5, 4), S(-5, 15), S(-10, 18), S(-2, 25), S(4, 13), S(8, 16), S(1, 16),
	S(-12, 3), S(-12, 5), S(-9, 13), S(-4, 34), S(-4, 35), S(-5, 34), S(8, 15), S(4, 39),
	S(-16, 18), S(-10, 19), S(-7, 18), S(-12, 39), S(1, 35), S(1, 50), S(12, 41), S(17, 23),
	S(-1, 8), S(-6, 13), S(-8, 36), S(-7, 44), S(10, 42), S(52, 45), S(68, 4), S(43, 44),
	S(-17, 8), S(-26, 12), S(-11, 26), S(-16, 46), S(1, 56), S(13, 34), S(3, 27), S(47, 31),
	S(-38, 19), S(-11, 2), S(0, 25), S(-14, 53), S(11, 29), S(27, 25), S(53, -5), S(0, 20),

	// 2.6 King PSQT
	S(41, -86), S(67, -62), S(40, -43), S(-48, -25), S(2, -40), S(-27, -24), S(44, -55), S(43, -86),
	S(53, -51), S(17, -19), S(4, -7), S(-27, 2), S(-35, 7), S(-20, 2), S(28, -20), S(30, -40),
	S(-22, -34), S(11, -11), S(-29, 13), S(-51, 28), S(-39, 27), S(-45, 20), S(-16, -2), S(-51, -13),
	S(-31, -34), S(-34, -2), S(-49, 29), S(-62, 43), S(-75, 46), S(-30, 30), S(-36, 13), S(-64, -11),
	S(-21, -21), S(-22, 7), S(-42, 33), S(-61, 46), S(-53, 45), S(-33, 39), S(-20, 18), S(-28, -8),
	S(-44, -18), S(8, 10), S(-38, 31), S(-38, 40), S(-23, 42), S(-8, 45), S(0, 28), S(-3, -2),
	S(-45, -29), S(-13, 4), S(-42, 14), S(-5, 14), S(-18, 26), S(3, 31), S(8, 24), S(18, -11),
	S(-43, -92), S(-29, -48), S(-21, -36), S(-51, -4), S(-26, -15), S(4, -19), S(8, -8), S(-21, -90),

	// 3. Piece mobility
	// Score tables depending on the pseudolegal moves available

	// 3.1 Knight mobility (0-8)
	S(-19, -24), S(-2, 4), S(8, 18), S(11, 28), S(15, 33), S(20, 38), S(26, 36), S(31, 31), S(31, 24),

	// 3.2 Bishop mobility (0-13)
	S(-21, -42), S(-11, -24), S(-1, -8), S(6, 6), S(12, 16), S(16, 28), S(20, 32),
	S(21, 35), S(22, 41), S(26, 36), S(32, 33), S(40, 31), S(37, 40), S(38, 24),

	// 3.3 Rook mobility (0-14)
	S(-17, -24), S(-8, -8), S(-4, -3), S(0, 0), S(0, 12), S(7, 16), S(9, 19), S(15, 19),
	S(18, 23), S(22, 28), S(25, 29), S(24, 34), S(34, 30), S(39, 27), S(41, 29),

	// 3.4 Queen mobility (0-27)
	S(-6, -89), S(-3, -115), S(-9, -92), S(-6, -69), S(-6, -55), S(-3, -38), S(-1, -22), S(1, -16), S(2, 0),
	S(6, -1), S(7, 5), S(10, 14), S(11, 21), S(12, 27), S(11, 38), S(14, 42), S(11, 58), S(17, 61),
	S(22, 65), S(36, 58), S(31, 77), S(74, 54), S(61, 66), S(72, 63), S(120, 70), S(98, 57), S(98, 79), S(84, 60),

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
	S(0, 0), S(-13, 14), S(-15, 19), S(-9, 46), S(16, 65), S(26, 78), S(41, 62), S(0, 0),

	// 5.2 Blocked passed pawn penalties by rank
	S(0, 0), S(-28, 4), S(-21, 4), S(-20, -14), S(-10, -35), S(-17, -80), S(-46, -100), S(0, 0),

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

	// 6.4 Pawn attacking minor and major pieces -- unused
	S(51, 28), S(56, 32),

	// 6.5 Tempo bonus
	S(18, 0),

	// 7. To be sorted

	// 7.1 Threat matrix
	// It somewhat helps to see beyond the horizon, also it ruins nps
	// Threatened king values are 0 due to the tuning dataset lacking such positions

	// Pawn attacking
	S(0, 0), S(47, -1), S(51, 28), S(64, -11), S(46, -13), S(0, 0),
	// Knight attacking
	S(-9, 3), S(0, 0), S(34, 14), S(52, -9), S(38, -24), S(0, 0),
	// Bishop attacking
	S(-6, 6), S(12, 20), S(0, 0), S(40, 9), S(58, 58), S(0, 0),
	// Rook attacking
	S(-6, 4), S(3, 6), S(13, 1), S(0, 0), S(61, 15), S(0, 0),
	// Queen attacking
	S(-6, 12), S(1, 3), S(0, 14), S(1, 7), S(0, 0), S(0, 0),
	// King attacking
	S(15, 27), S(-24, 9), S(-24, 18), S(-24, 3), S(0, 0), S(0, 0),

	// 7.2 Supported pawn bonus (by rank)
	S(0, 0), S(0, 0), S(9, 12), S(5, 4), S(-1, 6), S(7, 2), S(78, 33), S(0, 0),

	// 7.3 Pawn phalanx (by rank)
	S(2, 0), S(7, 0), S(1, -3), S(14, 17), S(36, 36), S(42, 42), S(42, 42), S(0, 0),
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
