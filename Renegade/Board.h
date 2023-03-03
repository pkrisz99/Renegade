#pragma once
#include "Move.h"
#include "Magics.h"
#include "Utils.cpp"

/*
* This is the board representation of Renegade.
* It also includes logic for move generation and handles repetition checks as well.
* A generic bitboard approach is used, pseudolegal moves are filtered via attack map generation.
*/

// Magic lookup tables
extern uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
extern uint64_t GetQueenAttacks(const int square, const uint64_t occupancy);

class Board
{

public:
	Board();
	Board(const Board &b);
	Board(const std::string fen);
	Board Copy();
	void Setup(const std::string fen);

	void Push(const Move move);
	bool PushUci(const std::string ucistr);
	const std::string GetFEN();

	const int GetPieceAt(const int place);
	const uint64_t GetOccupancy();
	const uint64_t GetOccupancy(const int pieceColor);
	const uint64_t Hash(const bool hashPlys);
	const uint64_t HashInternal();

	bool AreThereLegalMoves(const bool turn);
	bool IsLegalMove(const Move m, const int turn);
	void GeneratePseudoLegalMoves(std::vector<Move>& moves, const int turn, const bool quiescenceOnly);
	void GenerateLegalMoves(std::vector<Move>& moves, const int turn);
	void GenerateNonQuietMoves(std::vector<Move>& moves, const int turn);
	const uint64_t CalculateAttackedSquares(const int colorOfPieces);
	const bool IsMoveQuiet(const Move& move);
	const bool IsDraw();
	const GameState GetGameState();

	/*
	const uint64_t GenerateSlidingAttacksShiftUp(const int direction, const uint64_t boundMask, const uint64_t propagatingPieces,
		const uint64_t friendlyPieces, const uint64_t opponentPieces);
	const uint64_t GenerateSlidingAttacksShiftDown(const int direction, const uint64_t boundMask, const uint64_t propagatingPieces,
		const uint64_t friendlyPieces, const uint64_t opponentPieces);*/

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
	std::vector<uint64_t> PastHashes;
	uint64_t HashValue;
	int OccupancyInts[64];


private:
	const int GetPieceAtFromBitboards(const int place);
	void GenerateOccupancy();
	void TryMove(const Move move);

	void GenerateKnightMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	void GenerateKingMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	void GeneratePawnMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	void GenerateCastlingMoves(std::vector<Move>& moves);
	void GenerateSlidingMoves(std::vector<Move>& moves, const int piece, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy, const bool quiescenceOnly);

	const uint64_t GenerateKnightAttacks(const int from);
	const uint64_t GenerateKingAttacks(const int from);
	
};

