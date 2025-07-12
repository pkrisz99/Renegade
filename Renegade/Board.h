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

	uint64_t Threats = 0;
	uint64_t BoardHash = 0;
	uint64_t WhiteNonPawnHash = 0;
	uint64_t BlackNonPawnHash = 0;

	uint8_t HalfmoveClock = 0;
	uint16_t FullmoveClock = 0;
	int8_t EnPassantSquare = -1;

	bool WhiteRightToShortCastle = false;
	bool WhiteRightToLongCastle = false;
	bool BlackRightToShortCastle = false;
	bool BlackRightToLongCastle = false;

	std::array<uint8_t, 64> Mailbox{};
	bool Turn = Side::White;

	inline void AddPiece(const uint8_t piece, const uint8_t square) {
		assert(IsValidPiece(piece));
		assert(square >= 0 && square < 64);
		assert(Mailbox[square] == Piece::None);

		switch (piece) {
		case Piece::WhitePawn:
			SetBitTrue(WhitePawnBits, square);
			break;
		case Piece::WhiteKnight:
			SetBitTrue(WhiteKnightBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteKnight][square];
			break;
		case Piece::WhiteBishop:
			SetBitTrue(WhiteBishopBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteBishop][square];
			break;
		case Piece::WhiteRook:
			SetBitTrue(WhiteRookBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteRook][square];
			break;
		case Piece::WhiteQueen:
			SetBitTrue(WhiteQueenBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteQueen][square];
			break;
		case Piece::WhiteKing:
			SetBitTrue(WhiteKingBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteKing][square];
			break;
		case Piece::BlackPawn:
			SetBitTrue(BlackPawnBits, square);
			break;
		case Piece::BlackKnight:
			SetBitTrue(BlackKnightBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackKnight][square];
			break;
		case Piece::BlackBishop:
			SetBitTrue(BlackBishopBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackBishop][square];
			break;
		case Piece::BlackRook:
			SetBitTrue(BlackRookBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackRook][square];
			break;
		case Piece::BlackQueen:
			SetBitTrue(BlackQueenBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackQueen][square];
			break;
		case Piece::BlackKing:
			SetBitTrue(BlackKingBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackKing][square];
			break;
		default: assert(false); break;
		}

		Mailbox[square] = piece;
		BoardHash ^= Zobrist.PieceSquare[piece][square];
	}

	inline void RemovePiece(const uint8_t piece, const uint8_t square) {
		assert(IsValidPiece(piece));
		assert(square >= 0 && square < 64);
		assert(Mailbox[square] != Piece::None);
		
		switch (piece) {
		case Piece::WhitePawn:
			SetBitFalse(WhitePawnBits, square);
			break;
		case Piece::WhiteKnight:
			SetBitFalse(WhiteKnightBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteKnight][square];
			break;
		case Piece::WhiteBishop:
			SetBitFalse(WhiteBishopBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteBishop][square];
			break;
		case Piece::WhiteRook:
			SetBitFalse(WhiteRookBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteRook][square];
			break;
		case Piece::WhiteQueen:
			SetBitFalse(WhiteQueenBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteQueen][square];
			break;
		case Piece::WhiteKing:
			SetBitFalse(WhiteKingBits, square);
			WhiteNonPawnHash ^= Zobrist.PieceSquare[Piece::WhiteKing][square];
			break;
		case Piece::BlackPawn:
			SetBitFalse(BlackPawnBits, square);
			break;
		case Piece::BlackKnight:
			SetBitFalse(BlackKnightBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackKnight][square];
			break;
		case Piece::BlackBishop:
			SetBitFalse(BlackBishopBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackBishop][square];
			break;
		case Piece::BlackRook:
			SetBitFalse(BlackRookBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackRook][square];
			break;
		case Piece::BlackQueen:
			SetBitFalse(BlackQueenBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackQueen][square];
			break;
		case Piece::BlackKing:
			SetBitFalse(BlackKingBits, square);
			BlackNonPawnHash ^= Zobrist.PieceSquare[Piece::BlackKing][square];
			break;
		default: assert(false); break;
		}

		Mailbox[square] = Piece::None;
		BoardHash ^= Zobrist.PieceSquare[piece][square];
	}

	inline uint8_t GetPieceAt(const uint8_t square) const {
		assert(square >= 0 && square < 64);
		assert(IsValidPieceOrNone(Mailbox[square]));
		return Mailbox[square];
	}

	template<const bool state>
	inline void SetWhiteShortCastlingRight() {
		if (state != WhiteRightToShortCastle) {
			WhiteRightToShortCastle = state;
			BoardHash ^= Zobrist.Castling[0];
		}
	}

	template<const bool state>
	inline void SetWhiteLongCastlingRight() {
		if (state != WhiteRightToLongCastle) {
			WhiteRightToLongCastle = state;
			BoardHash ^= Zobrist.Castling[1];
		}
	}

	template<const bool state>
	inline void SetBlackShortCastlingRight() {
		if (state != BlackRightToShortCastle) {
			BlackRightToShortCastle = state;
			BoardHash ^= Zobrist.Castling[2];
		}
	}

	template<const bool state>
	inline void SetBlackLongCastlingRight() {
		if (state != BlackRightToLongCastle) {
			BlackRightToLongCastle = state;
			BoardHash ^= Zobrist.Castling[3];
		}
	}

	inline uint64_t GetOccupancy() const {
		return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits
			| BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
	}

	//uint64_t CalculateHash() const;
	void ApplyMove(const Move& move, const CastlingConfiguration& castling);

	[[maybe_unused]] uint64_t CalculateMaterialHash() const;
	uint64_t CalculatePawnHash() const;

};

static_assert(sizeof(Board) == 208);
