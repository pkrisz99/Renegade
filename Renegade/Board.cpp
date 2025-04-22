#include "Board.h"

uint64_t Board::CalculateHash() const {
	uint64_t boardHash = 0;

	uint64_t bits = WhitePawnBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::WhitePawn][sq];
	}
	bits = WhiteKnightBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::WhiteKnight][sq];
	}
	bits = WhiteBishopBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::WhiteBishop][sq];
	}
	bits = WhiteRookBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::WhiteRook][sq];
	}
	bits = WhiteQueenBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::WhiteQueen][sq];
	}
	int sq = LsbSquare(WhiteKingBits);
	boardHash ^= Zobrist.PieceSquare[Piece::WhiteKing][sq];

	bits = BlackPawnBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::BlackPawn][sq];
	}
	bits = BlackKnightBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::BlackKnight][sq];
	}
	bits = BlackBishopBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::BlackBishop][sq];
	}
	bits = BlackRookBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::BlackRook][sq];
	}
	bits = BlackQueenBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		boardHash ^= Zobrist.PieceSquare[Piece::BlackQueen][sq];
	}
	sq = LsbSquare(BlackKingBits);
	boardHash ^= Zobrist.PieceSquare[Piece::BlackKing][sq];

	// Castling, en passant, and side to move
	if (WhiteRightToShortCastle) boardHash ^= Zobrist.Castling[0];
	if (WhiteRightToLongCastle) boardHash ^= Zobrist.Castling[1];
	if (BlackRightToShortCastle) boardHash ^= Zobrist.Castling[2];
	if (BlackRightToLongCastle) boardHash ^= Zobrist.Castling[3];
	if (EnPassantSquare != -1) boardHash ^= Zobrist.EnPassant[GetSquareFile(EnPassantSquare)];
	if (Turn == Side::White) boardHash ^= Zobrist.SideToMove;

	return boardHash;
}

void Board::ApplyMove(const Move& move, const CastlingConfiguration& castling) {

	assert(!move.IsNull());
	const uint8_t piece = GetPieceAt(move.from);
	const uint8_t pieceType = TypeOfPiece(piece);
	const uint8_t capturedPiece = GetPieceAt(move.to);

	// Update bitboard fields for ordinary moves

	switch (capturedPiece) {
	case Piece::None: break;
	case Piece::WhitePawn: RemovePiece<Piece::WhitePawn>(move.to); break;
	case Piece::WhiteKnight: RemovePiece<Piece::WhiteKnight>(move.to); break;
	case Piece::WhiteBishop: RemovePiece<Piece::WhiteBishop>(move.to); break;
	case Piece::WhiteRook: RemovePiece<Piece::WhiteRook>(move.to); break;
	case Piece::WhiteQueen: RemovePiece<Piece::WhiteQueen>(move.to); break;
	case Piece::BlackPawn: RemovePiece<Piece::BlackPawn>(move.to); break;
	case Piece::BlackKnight: RemovePiece<Piece::BlackKnight>(move.to); break;
	case Piece::BlackBishop: RemovePiece<Piece::BlackBishop>(move.to); break;
	case Piece::BlackRook: RemovePiece<Piece::BlackRook>(move.to); break;
	case Piece::BlackQueen: RemovePiece<Piece::BlackQueen>(move.to); break;
	}

	switch (piece) {
	case Piece::WhitePawn: RemovePiece<Piece::WhitePawn>(move.from); AddPiece<Piece::WhitePawn>(move.to); break;
	case Piece::WhiteKnight: RemovePiece<Piece::WhiteKnight>(move.from); AddPiece<Piece::WhiteKnight>(move.to); break;
	case Piece::WhiteBishop: RemovePiece<Piece::WhiteBishop>(move.from); AddPiece<Piece::WhiteBishop>(move.to); break;
	case Piece::WhiteRook: RemovePiece<Piece::WhiteRook>(move.from); AddPiece<Piece::WhiteRook>(move.to); break;
	case Piece::WhiteQueen: RemovePiece<Piece::WhiteQueen>(move.from); AddPiece<Piece::WhiteQueen>(move.to); break;
	case Piece::WhiteKing: RemovePiece<Piece::WhiteKing>(move.from); AddPiece<Piece::WhiteKing>(move.to); break;
	case Piece::BlackPawn: RemovePiece<Piece::BlackPawn>(move.from); AddPiece<Piece::BlackPawn>(move.to); break;
	case Piece::BlackKnight: RemovePiece<Piece::BlackKnight>(move.from); AddPiece<Piece::BlackKnight>(move.to); break;
	case Piece::BlackBishop: RemovePiece<Piece::BlackBishop>(move.from); AddPiece<Piece::BlackBishop>(move.to); break;
	case Piece::BlackRook: RemovePiece<Piece::BlackRook>(move.from); AddPiece<Piece::BlackRook>(move.to); break;
	case Piece::BlackQueen: RemovePiece<Piece::BlackQueen>(move.from); AddPiece<Piece::BlackQueen>(move.to); break;
	case Piece::BlackKing: RemovePiece<Piece::BlackKing>(move.from); AddPiece<Piece::BlackKing>(move.to); break;
	}

	// Handle en passant
	if (move.to == EnPassantSquare) {
		if (piece == Piece::WhitePawn) RemovePiece<Piece::BlackPawn>(EnPassantSquare - 8);
		else if (piece == Piece::BlackPawn) RemovePiece<Piece::WhitePawn>(EnPassantSquare + 8);
	}

	// Handle promotions
	if (piece == Piece::WhitePawn) {
		switch (move.flag) {
		case MoveFlag::None: break;
		case MoveFlag::PromotionToQueen: RemovePiece<Piece::WhitePawn>(move.to); AddPiece<Piece::WhiteQueen>(move.to); break;
		case MoveFlag::PromotionToKnight: RemovePiece<Piece::WhitePawn>(move.to); AddPiece<Piece::WhiteKnight>(move.to); break;
		case MoveFlag::PromotionToRook: RemovePiece<Piece::WhitePawn>(move.to); AddPiece<Piece::WhiteRook>(move.to); break;
		case MoveFlag::PromotionToBishop: RemovePiece<Piece::WhitePawn>(move.to); AddPiece<Piece::WhiteBishop>(move.to); break;
		}
	}
	else if (piece == Piece::BlackPawn) {
		switch (move.flag) {
		case MoveFlag::None: break;
		case MoveFlag::PromotionToQueen: RemovePiece<Piece::BlackPawn>(move.to); AddPiece<Piece::BlackQueen>(move.to); break;
		case MoveFlag::PromotionToKnight: RemovePiece<Piece::BlackPawn>(move.to); AddPiece<Piece::BlackKnight>(move.to); break;
		case MoveFlag::PromotionToRook: RemovePiece<Piece::BlackPawn>(move.to); AddPiece<Piece::BlackRook>(move.to); break;
		case MoveFlag::PromotionToBishop: RemovePiece<Piece::BlackPawn>(move.to); AddPiece<Piece::BlackBishop>(move.to); break;
		}
	}

	// Handle castling
	if (piece == Piece::WhiteKing && capturedPiece == Piece::WhiteRook) {
		WhiteRightToShortCastle = false;
		WhiteRightToLongCastle = false;

		if (move.flag == MoveFlag::ShortCastle) {
			RemovePiece<Piece::WhiteKing>(move.to);
			AddPiece<Piece::WhiteKing>(Squares::G1);
			AddPiece<Piece::WhiteRook>(Squares::F1);
		}
		else {
			RemovePiece<Piece::WhiteKing>(move.to);
			AddPiece<Piece::WhiteKing>(Squares::C1);
			AddPiece<Piece::WhiteRook>(Squares::D1);
		}

	}
	else if (piece == Piece::BlackKing && capturedPiece == Piece::BlackRook) {
		BlackRightToShortCastle = false;
		BlackRightToLongCastle = false;

		if (move.flag == MoveFlag::ShortCastle) {
			RemovePiece<Piece::BlackKing>(move.to);
			AddPiece<Piece::BlackKing>(Squares::G8);
			AddPiece<Piece::BlackRook>(Squares::F8);
		}
		else {
			RemovePiece<Piece::BlackKing>(move.to);
			AddPiece<Piece::BlackKing>(Squares::C8);
			AddPiece<Piece::BlackRook>(Squares::D8);
		}
	}

	// TODO: branch when castling rights exist

	// Update castling rights
	if (piece == Piece::WhiteKing) {
		WhiteRightToShortCastle = false;
		WhiteRightToLongCastle = false;
	}
	else if (piece == Piece::BlackKing) {
		BlackRightToShortCastle = false;
		BlackRightToLongCastle = false;
	}
	else if (piece == Piece::WhiteRook) {
		if (move.from == castling.WhiteLongCastleRookSquare) WhiteRightToLongCastle = false;
		else if (move.from == castling.WhiteShortCastleRookSquare) WhiteRightToShortCastle = false;
	}
	else if (piece == Piece::BlackRook) {
		if (move.from == castling.BlackLongCastleRookSquare) BlackRightToLongCastle = false;
		else if (move.from == castling.BlackShortCastleRookSquare) BlackRightToShortCastle = false;
	}

	if (capturedPiece == Piece::WhiteRook) {
		if (move.to == castling.WhiteLongCastleRookSquare) WhiteRightToLongCastle = false;
		else if (move.to == castling.WhiteShortCastleRookSquare) WhiteRightToShortCastle = false;
	}
	else if (capturedPiece == Piece::BlackRook) {
		if (move.to == castling.BlackLongCastleRookSquare) BlackRightToLongCastle = false;
		else if (move.to == castling.BlackShortCastleRookSquare) BlackRightToShortCastle = false;
	}

	// Update en passant
	EnPassantSquare = -1;
	if (move.flag == MoveFlag::EnPassantPossible) {
		if (Turn == Side::White) {
			const bool pawnOnLeft = GetSquareFile(move.to) != 0 && GetPieceAt(move.to - 1) == Piece::BlackPawn;
			const bool pawnOnRight = GetSquareFile(move.to) != 7 && GetPieceAt(move.to + 1) == Piece::BlackPawn;
			if (pawnOnLeft || pawnOnRight) EnPassantSquare = move.to - 8;
		}
		else {
			const bool pawnOnLeft = GetSquareFile(move.to) != 0 && GetPieceAt(move.to - 1) == Piece::WhitePawn;
			const bool pawnOnRight = GetSquareFile(move.to) != 7 && GetPieceAt(move.to + 1) == Piece::WhitePawn;
			if (pawnOnLeft || pawnOnRight) EnPassantSquare = move.to + 8;
		}
	}

	// Update counters
	HalfmoveClock += 1;
	if (capturedPiece != Piece::None || pieceType == PieceType::Pawn) HalfmoveClock = 0;
	Turn = !Turn;
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
