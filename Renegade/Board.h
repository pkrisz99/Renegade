#pragma once
#include "Move.h"
#include "Magics.h"
#include "Utils.h"
#include <algorithm>

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
	Board(const std::string& fen);
	Board() : Board(FEN::StartPos) {};
	Board(const Board& b);

	void Push(const Move& move);
	bool PushUci(const std::string& ucistr);

	uint64_t GetOccupancy() const;
	uint64_t GetOccupancy(const bool side) const;
	uint8_t GetPieceAt(const uint8_t place) const;
	uint64_t Hash() const;
	uint64_t HashInternal();

	void GenerateMoves(std::vector<Move>& moves, const MoveGen moveGen, const Legality legality);
	template <bool attackingSide> uint64_t CalculateAttackedSquaresTemplated() const;
	uint64_t CalculateAttackedSquares(const bool attackingSide) const;
	bool IsLegalMove(const Move& m);
	bool IsMoveQuiet(const Move& move) const;
	template <bool attackingSide> bool IsSquareAttacked(const uint8_t square) const;

	bool IsDraw(const bool threefold) const;
	GameState GetGameState();
	int GetPlys() const;
	std::string GetFEN() const;
	bool IsInCheck() const;

	template <bool side> uint8_t GetKingSquare() const;
	uint64_t GetAttackersOfSquare(const uint8_t square, const uint64_t occupied) const;

	template <bool side>
	inline uint64_t GetPawnAttacks() const {
		if constexpr (side == Side::White) return ((WhitePawnBits & ~File[0]) << 7) | ((WhitePawnBits & ~File[7]) << 9);
		else return ((BlackPawnBits & ~File[0]) >> 9) | ((BlackPawnBits & ~File[7]) >> 7);
	}

	inline bool ShouldNullMovePrune() const {
		const int friendlyPieces = Popcount(GetOccupancy(Turn));
		const int friendlyPawns = (Turn == Side::White) ? Popcount(WhitePawnBits) : Popcount(BlackPawnBits);
		return (friendlyPieces - friendlyPawns) > 1;
	}


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
	int8_t EnPassantSquare;
	uint8_t HalfmoveClock;
	bool WhiteRightToShortCastle;
	bool WhiteRightToLongCastle;
	bool BlackRightToShortCastle;
	bool BlackRightToLongCastle;
	bool Turn;

private:
	void GenerateOccupancy();
	void TryMove(const Move& move);

	template <bool side, MoveGen moveGen> void GeneratePseudolegalMoves(std::vector<Move>& moves) const;
	template <bool side, MoveGen moveGen> void GenerateKnightMoves(std::vector<Move>& moves, const int home) const;
	template <bool side, MoveGen moveGen> void GenerateKingMoves(std::vector<Move>& moves, const int home) const;
	template <bool side, MoveGen moveGen> void GeneratePawnMoves(std::vector<Move>& moves, const int home) const;
	template <bool side> void GenerateCastlingMoves(std::vector<Move>& moves) const;
	template <bool side, int pieceType, MoveGen moveGen> void GenerateSlidingMoves(std::vector<Move>& moves, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy) const;

	uint64_t GenerateKnightAttacks(const int from) const;
	uint64_t GenerateKingAttacks(const int from) const;
	
};

