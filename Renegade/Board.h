#pragma once
#include "Move.h"
#include "Utils.cpp"

/*
* This is the board representation of Renegade.
* Square 0 = A1, 1 = B1 ... 8 = A2 ... 63 = H8
*/

class Board
{

public:
	Board();
	Board(const Board &b);
	Board(const std::string fen);
	void Push(const Move move);
	bool PushUci(const std::string ucistr);
	Board Copy();
	const void Draw(const uint64_t customBits);
	const int GetPieceAt(const int place);
	const int GetPieceAtFromBitboards(const int place);
	void GenerateOccupancy();
	const uint64_t Hash(const bool hashPlys);
	const uint64_t HashInternal();
	const uint64_t GetOccupancy();
	const uint64_t GetOccupancy(const int pieceColor);

	bool AreThereLegalMoves(const bool turn, const uint64_t previousAttackMap);
	bool IsLegalMove(const Move m, const int turn);
	void TryMove(const Move move);
	void GeneratePseudoLegalMoves(std::vector<Move>& moves, const int turn, const bool quiescenceOnly);
	void GenerateLegalMoves(std::vector<Move>& moves, const int turn);
	void GenerateNonQuietMoves(std::vector<Move>& moves, const int turn);
	uint64_t CalculateAttackedSquares(const int turn);

	void GenerateKnightMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	void GenerateKingMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	void GeneratePawnMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	void GenerateCastlingMoves(std::vector<Move>& moves);
	void GenerateSlidingMoves(std::vector<Move>& moves, const int piece, const int home, const bool quiescenceOnly);

	const uint64_t GenerateKnightAttacks(const int from);
	const uint64_t GenerateKingAttacks(const int from);
	const uint64_t GenerateSlidingAttacksShiftUp(const int direction, const uint64_t boundMask, const uint64_t propagatingPieces,
		const uint64_t friendlyPieces, const uint64_t opponentPieces);
	const uint64_t GenerateSlidingAttacksShiftDown(const int direction, const uint64_t boundMask, const uint64_t propagatingPieces,
		const uint64_t friendlyPieces, const uint64_t opponentPieces);

	// Board variables:
	uint64_t WhitePawnBits;
	uint64_t WhiteKnightBits;
	uint64_t WhiteBishopBits;
	uint64_t WhiteRookBits;
	uint64_t WhiteQueenBits;
	uint64_t WhiteKingBits;
	uint64_t BlackPawnBits;
	uint64_t BlackKnightBits;
	uint64_t BlackBishopBits;
	uint64_t BlackRookBits;
	uint64_t BlackQueenBits;
	uint64_t BlackKingBits;
	uint64_t AttackedSquares;
	int EnPassantSquare;
	bool WhiteRightToShortCastle;
	bool WhiteRightToLongCastle;
	bool BlackRightToShortCastle;
	bool BlackRightToLongCastle;
	bool Turn;
	int HalfmoveClock;
	int FullmoveClock;
	GameState State;
	std::vector<uint64_t> PastHashes;
	uint64_t HashValue;
	int OccupancyInts[64];

	// Board settings:
	bool StartingPosition;
	bool DrawCheck = true;

};

