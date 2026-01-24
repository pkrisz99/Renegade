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

	template<const uint8_t piece>
	inline void AddPiece(const uint8_t square) {
		assert(IsValidPiece(piece));
		assert(square >= 0 && square < 64);
		SetBitTrue(PieceBitboard(piece), square);
		Mailbox[square] = piece;
		BoardHash ^= Zobrist.PieceSquare[piece][square];
		if constexpr (ColorOfPiece(piece) == PieceColor::White && IsNonPawn(piece)) WhiteNonPawnHash ^= Zobrist.PieceSquare[piece][square];
		if constexpr (ColorOfPiece(piece) == PieceColor::Black && IsNonPawn(piece)) BlackNonPawnHash ^= Zobrist.PieceSquare[piece][square];
	}

	template<const uint8_t piece>
	inline void RemovePiece(const uint8_t square) {
		assert(IsValidPiece(piece));
		assert(square >= 0 && square < 64);
		SetBitFalse(PieceBitboard(piece), square);
		Mailbox[square] = Piece::None;
		BoardHash ^= Zobrist.PieceSquare[piece][square];
		if constexpr (ColorOfPiece(piece) == PieceColor::White && IsNonPawn(piece)) WhiteNonPawnHash ^= Zobrist.PieceSquare[piece][square];
		if constexpr (ColorOfPiece(piece) == PieceColor::Black && IsNonPawn(piece)) BlackNonPawnHash ^= Zobrist.PieceSquare[piece][square];
	}

	inline uint8_t GetPieceAt(const uint8_t square) const {
		assert(square >= 0 && square < 64);
		assert(IsValidPieceOrNone(Mailbox[square]));
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
		default: assert(false); return WhitePawnBits; // return something to silence warnings
		}
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

	void ApplyMove(const Move& move, const CastlingConfiguration& castling);

	[[maybe_unused]] uint64_t CalculateMaterialHash() const;
	uint64_t CalculatePawnHash() const;

	inline bool IsSquareThreatened(const uint8_t sq) const {
		return CheckBit(Threats, sq);
	}

};

static_assert(sizeof(Board) == 208);
