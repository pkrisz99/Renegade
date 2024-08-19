#pragma once
#include "Move.h"
#include "Utils.h"

// Magic lookup tables
uint64_t GetBishopAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetRookAttacks(const uint8_t square, const uint64_t occupancy);
uint64_t GetQueenAttacks(const uint8_t square, const uint64_t occupancy);

struct CastlingConfiguration {
	uint8_t WhiteLongCastleRookSquare = 0;
	uint8_t WhiteShortCastleRookSquare = 0;
	uint8_t BlackLongCastleRookSquare = 0;
	uint8_t BlackShortCastleRookSquare = 0;
};


struct Board {

	Board() = default;
	~Board() = default;
	Board(const Board& b);

	// Piece bitboards
	uint64_t WhitePawnBits = 0;
	uint64_t WhiteKnightBits = 0;
	uint64_t WhiteBishopBits = 0;
	uint64_t WhiteRookBits = 0;
	uint64_t WhiteQueenBits = 0;
	uint64_t WhiteKingBits = 0;
	uint64_t BlackPawnBits = 0;
	uint64_t BlackKnightBits = 0;
	uint64_t BlackBishopBits = 0;
	uint64_t BlackRookBits = 0;
	uint64_t BlackQueenBits = 0;
	uint64_t BlackKingBits = 0;

	uint8_t HalfmoveClock = 0;
	uint16_t FullmoveClock = 0;
	int8_t EnPassantSquare = -1;

	bool WhiteRightToShortCastle = false;
	bool WhiteRightToLongCastle = false;
	bool BlackRightToShortCastle = false;
	bool BlackRightToLongCastle = false;

	std::array<uint8_t, 64> Mailbox{};
	bool Turn = Side::White;

	template<const uint8_t piece>
	inline void AddPiece(const uint8_t square) {
		SetBitTrue(PieceBitboard(piece), square);
		Mailbox[square] = piece;
	}

	inline void AddPiece(const uint8_t piece, const uint8_t square) {
		SetBitTrue(PieceBitboard(piece), square);
		Mailbox[square] = piece;
	}

	template<const uint8_t piece>
	inline void RemovePiece(const uint8_t square) {
		SetBitFalse(PieceBitboard(piece), square);
		Mailbox[square] = Piece::None;
	}

	inline uint8_t GetPieceAt(const uint8_t square) const {
		assert(square >= 0 && square < 64);
		return Mailbox[square];
	}

	constexpr uint64_t& PieceBitboard(const uint8_t piece) {
		switch (piece) {
		case Piece::WhitePawn: return WhitePawnBits;
		case Piece::WhiteKnight: return WhiteKnightBits;
		case Piece::WhiteBishop: return WhiteBishopBits;
		case Piece::WhiteRook: return WhiteRookBits;
		case Piece::WhiteQueen: return WhiteQueenBits;
		case Piece::WhiteKing: return WhiteKingBits;
		case Piece::BlackPawn: return BlackPawnBits;
		case Piece::BlackKnight: return BlackKnightBits;
		case Piece::BlackBishop: return BlackBishopBits;
		case Piece::BlackRook: return BlackRookBits;
		case Piece::BlackQueen: return BlackQueenBits;
		case Piece::BlackKing: return BlackKingBits;
		default: assert(false);
		}
	}

	inline uint64_t GetOccupancy() const {
		return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits
			| BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
	}

	uint64_t CalculateHash() const;
	void ApplyMove(const Move& move, const CastlingConfiguration& castling);

    uint64_t CalculateMaterialKey() const;

};

static_assert(sizeof(Board) == 176);
