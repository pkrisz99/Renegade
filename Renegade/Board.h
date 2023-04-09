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

	const uint64_t GetOccupancy();
	const uint64_t GetOccupancy(const uint8_t pieceColor);
	const uint8_t GetPieceAt(const uint8_t place);
	const uint64_t Hash();
	const uint64_t HashInternal();

	const void GeneratePseudoLegalMoves(std::vector<Move>& moves, const bool turn, const bool quiescenceOnly);
	const void GenerateLegalMoves(std::vector<Move>& moves, const bool turn);
	const void GenerateNonQuietMoves(std::vector<Move>& moves, const bool turn);
	const uint64_t CalculateAttackedSquares(const uint8_t colorOfPieces);
	bool IsLegalMove(const Move m, const bool turn);
	const bool IsMoveQuiet(const Move& move);

	template <bool attackingSide>
	bool IsSquareAttacked(const uint8_t square);

	const bool AreThereLegalMoves(const bool turn);
	const bool IsDraw();
	const GameState GetGameState();
	const int GetPlys();
	const std::string GetFEN();
	const uint8_t IsInCheck();

	template <bool side>
	const uint8_t GetKingSquare();


	// Board variables:
	std::vector<uint64_t> PreviousHashes;
	uint8_t OccupancyInts[64];
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
	//uint64_t AttackedSquares;
	uint64_t HashValue;
	uint16_t FullmoveClock;
	uint8_t EnPassantSquare;
	uint8_t HalfmoveClock;
	bool WhiteRightToShortCastle;
	bool WhiteRightToLongCastle;
	bool BlackRightToShortCastle;
	bool BlackRightToLongCastle;
	bool Turn;

private:
	void GenerateOccupancy();
	void TryMove(const Move move);

	const void GenerateKnightMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	const void GenerateKingMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	const void GeneratePawnMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly);
	const void GenerateCastlingMoves(std::vector<Move>& moves);
	const void GenerateSlidingMoves(std::vector<Move>& moves, const int piece, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy, const bool quiescenceOnly);

	const uint64_t GenerateKnightAttacks(const int from);
	const uint64_t GenerateKingAttacks(const int from);
	
};

