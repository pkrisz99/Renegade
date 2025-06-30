#include "Board.h"

void Board::ApplyMove(const Move& move, const CastlingConfiguration& castling) {

	assert(!move.IsNull());
	const uint8_t piece = GetPieceAt(move.from);
	const uint8_t pieceType = TypeOfPiece(piece);
	const uint8_t capturedPiece = GetPieceAt(move.to);

	// Remove captured piece
	if (capturedPiece != Piece::None) RemovePiece(capturedPiece, move.to);

	// Move the moved piece
	RemovePiece(piece, move.from);
	AddPiece(piece, move.to);

	// Handle en passant
	if (move.to == EnPassantSquare) {
		if (piece == Piece::WhitePawn) RemovePiece(Piece::BlackPawn, EnPassantSquare - 8);
		else if (piece == Piece::BlackPawn) RemovePiece(Piece::WhitePawn, EnPassantSquare + 8);
	}

	// Handle promotions
	if (move.IsPromotion()) {
		if (piece == Piece::WhitePawn) {
			RemovePiece(Piece::WhitePawn, move.to);
			AddPiece(move.GetPromotionPieceType() + Piece::WhitePieceOffset, move.to);
		}
		else {
			RemovePiece(Piece::BlackPawn, move.to);
			AddPiece(move.GetPromotionPieceType() + Piece::BlackPieceOffset, move.to);
		}
	}

	// Handle castling
	if (piece == Piece::WhiteKing && capturedPiece == Piece::WhiteRook) {
		if (WhiteRightToShortCastle) {
			WhiteRightToShortCastle = false;
			BoardHash ^= Zobrist.Castling[0];
		}
		if (WhiteRightToLongCastle) {
			WhiteRightToLongCastle = false;
			BoardHash ^= Zobrist.Castling[1];
		}

		if (move.flag == MoveFlag::ShortCastle) {
			RemovePiece(Piece::WhiteKing, move.to);
			AddPiece(Piece::WhiteKing, Squares::G1);
			AddPiece(Piece::WhiteRook, Squares::F1);
		}
		else {
			RemovePiece(Piece::WhiteKing, move.to);
			AddPiece(Piece::WhiteKing, Squares::C1);
			AddPiece(Piece::WhiteRook, Squares::D1);
		}

	}
	else if (piece == Piece::BlackKing && capturedPiece == Piece::BlackRook) {
		if (BlackRightToShortCastle) {
			BlackRightToShortCastle = false;
			BoardHash ^= Zobrist.Castling[2];
		}
		if (BlackRightToLongCastle) {
			BlackRightToLongCastle = false;
			BoardHash ^= Zobrist.Castling[3];
		}

		if (move.flag == MoveFlag::ShortCastle) {
			RemovePiece(Piece::BlackKing, move.to);
			AddPiece(Piece::BlackKing, Squares::G8);
			AddPiece(Piece::BlackRook, Squares::F8);
		}
		else {
			RemovePiece(Piece::BlackKing, move.to);
			AddPiece(Piece::BlackKing, Squares::C8);
			AddPiece(Piece::BlackRook, Squares::D8);
		}
	}

	// TODO: branch when castling rights exist

	// Update castling rights from non-castling moves
	if (piece == Piece::WhiteKing) {
		SetWhiteShortCastlingRight<false>();
		SetWhiteLongCastlingRight<false>();
	}
	else if (piece == Piece::BlackKing) {
		SetBlackShortCastlingRight<false>();
		SetBlackLongCastlingRight<false>();
	}
	else if (piece == Piece::WhiteRook) {
		if (move.from == castling.WhiteShortCastleRookSquare) SetWhiteShortCastlingRight<false>();
		else if (move.from == castling.WhiteLongCastleRookSquare) SetWhiteLongCastlingRight<false>();
	}
	else if (piece == Piece::BlackRook) {
		if (move.from == castling.BlackShortCastleRookSquare) SetBlackShortCastlingRight<false>();
		else if (move.from == castling.BlackLongCastleRookSquare) SetBlackLongCastlingRight<false>();
	}

	if (capturedPiece == Piece::WhiteRook) {
		if (move.to == castling.WhiteShortCastleRookSquare) SetWhiteShortCastlingRight<false>();
		else if (move.to == castling.WhiteLongCastleRookSquare) SetWhiteLongCastlingRight<false>();
	}
	else if (capturedPiece == Piece::BlackRook) {
		if (move.to == castling.BlackShortCastleRookSquare) SetBlackShortCastlingRight<false>();
		else if (move.to == castling.BlackLongCastleRookSquare) SetBlackLongCastlingRight<false>();
	}

	// Update en passant
	if (EnPassantSquare != -1) {
		BoardHash ^= Zobrist.EnPassant[GetSquareFile(EnPassantSquare)];
		EnPassantSquare = -1;
	}
	
	if (move.flag == MoveFlag::EnPassantPossible) {
		if (Turn == Side::White) {
			const bool pawnOnLeft = GetSquareFile(move.to) != 0 && GetPieceAt(move.to - 1) == Piece::BlackPawn;
			const bool pawnOnRight = GetSquareFile(move.to) != 7 && GetPieceAt(move.to + 1) == Piece::BlackPawn;
			if (pawnOnLeft || pawnOnRight) {
				EnPassantSquare = move.to - 8;
				BoardHash ^= Zobrist.EnPassant[GetSquareFile(EnPassantSquare)];
			}
		}
		else {
			const bool pawnOnLeft = GetSquareFile(move.to) != 0 && GetPieceAt(move.to - 1) == Piece::WhitePawn;
			const bool pawnOnRight = GetSquareFile(move.to) != 7 && GetPieceAt(move.to + 1) == Piece::WhitePawn;
			if (pawnOnLeft || pawnOnRight) {
				EnPassantSquare = move.to + 8;
				BoardHash ^= Zobrist.EnPassant[GetSquareFile(EnPassantSquare)];
			}
		}
	}

	// Update counters
	HalfmoveClock += 1;
	if (capturedPiece != Piece::None || pieceType == PieceType::Pawn) HalfmoveClock = 0;
	Turn = !Turn;
	BoardHash ^= Zobrist.SideToMove;
	if (Turn == Side::White) FullmoveClock += 1;

	assert(Popcount(WhiteKingBits) == 1 && Popcount(BlackKingBits) == 1);
}

uint64_t Board::CalculateMaterialHash() const {
	uint64_t materialHash = 0;
	materialHash |= static_cast<uint64_t>(Popcount(WhitePawnBits));
	materialHash |= static_cast<uint64_t>(Popcount(WhiteKnightBits)) << 6;
	materialHash |= static_cast<uint64_t>(Popcount(WhiteBishopBits)) << 12;
	materialHash |= static_cast<uint64_t>(Popcount(WhiteRookBits)) << 18;
	materialHash |= static_cast<uint64_t>(Popcount(WhiteQueenBits)) << 24;
	materialHash |= static_cast<uint64_t>(Popcount(BlackPawnBits)) << 30;
	materialHash |= static_cast<uint64_t>(Popcount(BlackKnightBits)) << 36;
	materialHash |= static_cast<uint64_t>(Popcount(BlackBishopBits)) << 42;
	materialHash |= static_cast<uint64_t>(Popcount(BlackRookBits)) << 48;
	materialHash |= static_cast<uint64_t>(Popcount(BlackQueenBits)) << 54;
	return MurmurHash3(materialHash);
}

uint64_t Board::CalculatePawnHash() const {
	return MurmurHash3(WhitePawnBits) ^ MurmurHash3(BlackPawnBits ^ Zobrist.SideToMove);
}
