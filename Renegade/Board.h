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
	Board(const Board& b);
	Board(const std::string fen);
	Board Copy();
	void Setup(const std::string fen);

	void Push(const Move move);
	bool PushUci(const std::string ucistr);

	const uint64_t GetOccupancy() const;
	const uint64_t GetOccupancy(const uint8_t pieceColor) const;
	const uint8_t GetPieceAt(const uint8_t place) const;
	const uint64_t Hash() const;
	const uint64_t HashInternal();

	const void GenerateMoves(std::vector<Move>& moves, const MoveGen moveGen, const Legality legality);
	const uint64_t CalculateAttackedSquares(const uint8_t colorOfPieces) const;
	bool IsLegalMove(const Move m);
	const bool IsMoveQuiet(const Move& move) const;
	template <bool attackingSide> bool IsSquareAttacked(const uint8_t square) const;

	const bool AreThereLegalMoves();
	const bool IsDraw() const;
	const GameState GetGameState();
	const int GetPlys() const;
	const std::string GetFEN() const;
	const bool IsInCheck() const;

	template <bool side> const uint8_t GetKingSquare() const;
	const uint64_t GetAttackersOfSquare(const uint8_t square) const;


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
	void TryMove(const Move& move);

	template <bool side, MoveGen moveGen> const void GeneratePseudolegalMoves(std::vector<Move>& moves) const;
	template <bool side, MoveGen moveGen> const void GenerateKnightMoves(std::vector<Move>& moves, const int home) const;
	template <bool side, MoveGen moveGen> const void GenerateKingMoves(std::vector<Move>& moves, const int home) const;
	template <bool side, MoveGen moveGen> const void GeneratePawnMoves(std::vector<Move>& moves, const int home) const;
	template <bool side> const void GenerateCastlingMoves(std::vector<Move>& moves) const;
	template <bool side, int pieceType, MoveGen moveGen> const void GenerateSlidingMoves(std::vector<Move>& moves, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy) const;

	const uint64_t GenerateKnightAttacks(const int from) const;
	const uint64_t GenerateKingAttacks(const int from) const;
	
};

