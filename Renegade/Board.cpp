#include "Board.h"

#pragma once

// Constructors and related -----------------------------------------------------------------------

void Board::Setup(const std::string fen) {
	WhitePawnBits = 0L;
	WhiteKnightBits = 0L;
	WhiteBishopBits = 0L;
	WhiteRookBits = 0L;
	WhiteQueenBits = 0L;
	WhiteKingBits = 0L;
	BlackPawnBits = 0L;
	BlackKnightBits = 0L;
	BlackBishopBits = 0L;
	BlackRookBits = 0L;
	BlackQueenBits = 0L;
	BlackKingBits = 0L;
	EnPassantSquare = -1;
	WhiteRightToShortCastle = false;
	WhiteRightToLongCastle = false;
	BlackRightToShortCastle = false;
	BlackRightToLongCastle = false;
	HalfmoveClock = 0;
	FullmoveClock = 0;
	Turn = Turn::White;
	PreviousHashes = std::vector<uint64_t>();

	std::vector<std::string> parts = Split(fen);
	int place = 56;

	for (char f : parts[0]) {
		switch (f) {
		case '/': place -= 16; break;
		case '1': place += 1; break;
		case '2': place += 2; break;
		case '3': place += 3; break;
		case '4': place += 4; break;
		case '5': place += 5; break;
		case '6': place += 6; break;
		case '7': place += 7; break;
		case '8': place += 8; break;
		case 'P': SetBitTrue(WhitePawnBits, place); place += 1; break;
		case 'N': SetBitTrue(WhiteKnightBits, place); place += 1; break;
		case 'B': SetBitTrue(WhiteBishopBits, place); place += 1; break;
		case 'R': SetBitTrue(WhiteRookBits, place); place += 1; break;
		case 'Q': SetBitTrue(WhiteQueenBits, place); place += 1; break;
		case 'K': SetBitTrue(WhiteKingBits, place); place += 1; break;
		case 'p': SetBitTrue(BlackPawnBits, place); place += 1; break;
		case 'n': SetBitTrue(BlackKnightBits, place); place += 1; break;
		case 'b': SetBitTrue(BlackBishopBits, place); place += 1; break;
		case 'r': SetBitTrue(BlackRookBits, place); place += 1; break;
		case 'q': SetBitTrue(BlackQueenBits, place); place += 1; break;
		case 'k': SetBitTrue(BlackKingBits, place); place += 1; break;
		}
	}

	if (parts[1] == "w") Turn = Turn::White;
	else Turn = Turn::Black;

	for (char f : parts[2]) {
		switch (f) {
		case 'K': WhiteRightToShortCastle = true; break;
		case 'Q': WhiteRightToLongCastle = true; break;
		case 'k': BlackRightToShortCastle = true; break;
		case 'q': BlackRightToLongCastle = true; break;
		}
	}

	if (parts[3] != "-") {
		EnPassantSquare = SquareToNum(parts[3]);
	}

	HalfmoveClock = stoi(parts[4]);
	FullmoveClock = stoi(parts[5]);
	HashValue = HashInternal();
	PreviousHashes.push_back(HashValue);
	GenerateOccupancy();
}

Board::Board() {
	Setup(FEN::StartPos);
}

Board::Board(const std::string fen) {
	Setup(fen);
}

Board::Board(const Board& b) {
	WhitePawnBits = b.WhitePawnBits;
	WhiteKnightBits = b.WhiteKnightBits;
	WhiteBishopBits = b.WhiteBishopBits;
	WhiteRookBits = b.WhiteRookBits;
	WhiteQueenBits = b.WhiteQueenBits;
	WhiteKingBits = b.WhiteKingBits;

	BlackPawnBits = b.BlackPawnBits;
	BlackKnightBits = b.BlackKnightBits;
	BlackBishopBits = b.BlackBishopBits;
	BlackRookBits = b.BlackRookBits;
	BlackQueenBits = b.BlackQueenBits;
	BlackKingBits = b.BlackKingBits;

	EnPassantSquare = b.EnPassantSquare;
	WhiteRightToShortCastle = b.WhiteRightToShortCastle;
	WhiteRightToLongCastle = b.WhiteRightToLongCastle;
	BlackRightToShortCastle = b.BlackRightToShortCastle;
	BlackRightToLongCastle = b.BlackRightToLongCastle;
	Turn = b.Turn;
	HalfmoveClock = b.HalfmoveClock;
	FullmoveClock = b.FullmoveClock;
	std::copy(std::begin(b.OccupancyInts), std::end(b.OccupancyInts), std::begin(OccupancyInts));

	HashValue = b.HashValue;
	PreviousHashes.reserve(b.PreviousHashes.size() + 1);
	PreviousHashes = b.PreviousHashes;
}

Board Board::Copy() {
	return Board(*this);
}

// Generating board hash --------------------------------------------------------------------------

const uint64_t Board::HashInternal() {
	uint64_t hash = 0;

	uint64_t bits = WhitePawnBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 1 + sq];
	}
	bits = BlackPawnBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 0 + sq];
	}
	bits = WhiteKnightBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 3 + sq];
	}
	bits = BlackKnightBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 2 + sq];
	}
	bits = WhiteBishopBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 5 + sq];
	}
	bits = BlackBishopBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 4 + sq];
	}
	bits = WhiteRookBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 7 + sq];
	}
	bits = BlackRookBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 6 + sq];
	}
	bits = WhiteQueenBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 9 + sq];
	}
	bits = BlackQueenBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		hash ^= Zobrist[64 * 8 + sq];
	}

	int sq = 63 - Lzcount(WhiteKingBits);
	hash ^= Zobrist[64 * 11 + sq];

	sq = 63 - Lzcount(BlackKingBits);
	hash ^= Zobrist[64 * 10 + sq];
	
	// Castling
	if (WhiteRightToShortCastle) hash ^= Zobrist[768];
	if (WhiteRightToLongCastle) hash ^= Zobrist[769];
	if (BlackRightToShortCastle) hash ^= Zobrist[770];
	if (BlackRightToLongCastle) hash ^= Zobrist[771];

	// En passant
	if (EnPassantSquare != -1) {
		bool hasPawn = false;
		if (GetSquareFile(EnPassantSquare) != 0) {
			if (Turn == Turn::White) hasPawn = CheckBit(WhitePawnBits, EnPassantSquare - 7);
			else hasPawn = CheckBit(BlackPawnBits, EnPassantSquare + 9);
		}
		if ((GetSquareFile(EnPassantSquare) != 7) && !hasPawn) {
			if (Turn == Turn::White) hasPawn = CheckBit(WhitePawnBits, EnPassantSquare - 9);
			else hasPawn = CheckBit(BlackPawnBits, EnPassantSquare + 7);
		}
		if (hasPawn) hash ^= Zobrist[772 + GetSquareFile(EnPassantSquare)];
	}

	// Turn
	if (Turn == Turn::White) hash ^= Zobrist[780];

	return hash;
}

const uint64_t Board::Hash() const {
	return HashValue;
}

// Board occupancy --------------------------------------------------------------------------------

void Board::GenerateOccupancy() {
	std::fill(std::begin(OccupancyInts), std::end(OccupancyInts), 0);

	uint64_t bits = WhitePawnBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::WhitePawn;
	}
	bits = BlackPawnBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::BlackPawn;
	}
	bits = WhiteKnightBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::WhiteKnight;
	}
	bits = BlackKnightBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::BlackKnight;
	}
	bits = WhiteBishopBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::WhiteBishop;
	}
	bits = BlackBishopBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::BlackBishop;
	}
	bits = WhiteRookBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::WhiteRook;
	}
	bits = BlackRookBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::BlackRook;
	}
	bits = WhiteQueenBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::WhiteQueen;
	}
	bits = BlackQueenBits;
	while (bits != 0) {
		const int sq = Popsquare(bits);
		OccupancyInts[sq] = Piece::BlackQueen;
	}

	int sq = 63 - Lzcount(WhiteKingBits);
	OccupancyInts[sq] = Piece::WhiteKing;

	sq = 63 - Lzcount(BlackKingBits);
	OccupancyInts[sq] = Piece::BlackKing;
}

const uint64_t Board::GetOccupancy() const {
	return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits
		| BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

const uint64_t Board::GetOccupancy(const uint8_t pieceColor) const {
	if (pieceColor == PieceColor::White) return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
	return BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

const uint8_t Board::GetPieceAt(const uint8_t place) const {
	return OccupancyInts[place];
}

// Making moves -----------------------------------------------------------------------------------

// Converts the uci move input to the engine's own representation, and then plays it
bool Board::PushUci(const std::string ucistr) {
	const int sq1 = SquareToNum(ucistr.substr(0, 2));
	const int sq2 = SquareToNum(ucistr.substr(2, 2));
	const int extra = ucistr[4];
	Move move = Move(sq1, sq2);
	const int piece = GetPieceAt(sq1);
	const int capturedPiece = GetPieceAt(sq2);

	// Promotions
	if (extra == 'q') move.flag = MoveFlag::PromotionToQueen;
	else if (extra == 'r') move.flag = MoveFlag::PromotionToRook;
	else if (extra == 'b') move.flag = MoveFlag::PromotionToBishop;
	else if (extra == 'n') move.flag = MoveFlag::PromotionToKnight;

	// Castling
	if ((piece == Piece::WhiteKing) && (sq1 == 4) && (sq2 == 2)) move.flag = MoveFlag::LongCastle;
	else if ((piece == Piece::WhiteKing) && (sq1 == 4) && (sq2 == 6)) move.flag = MoveFlag::ShortCastle;
	else if ((piece == Piece::BlackKing) && (sq1 == 60) && (sq2 == 58)) move.flag = MoveFlag::LongCastle;
	else if ((piece == Piece::BlackKing) && (sq1 == 60) && (sq2 == 62)) move.flag = MoveFlag::ShortCastle;

	// En passant possibility
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		int f1 = sq1 / 8;
		int f2 = sq2 / 8;
		if (abs(f2 - f1) > 1) move.flag = MoveFlag::EnPassantPossible;
	}

	// En passant performed
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		if ((TypeOfPiece(capturedPiece) == 0) && (GetSquareFile(sq1) != GetSquareFile(sq2))) {
			move.flag = MoveFlag::EnPassantPerformed;
		}
	}

	// Generate the list of valid moves
	std::vector<Move> legalMoves;
	legalMoves.reserve(7);
	GenerateMoves(legalMoves, MoveGen::All, Legality::Legal);
	bool valid = false;
	for (const Move &m : legalMoves) {
		if ((m.to == move.to) && (m.from == move.from) && (m.flag == move.flag)) {
			valid = true;
			break;
		}
	}

	// Make the move if valid
	if (valid) {
		Push(move);
		return true;
	} else {
		return false;
	}

}

// Updating bitboard fields
void Board::TryMove(const Move& move) {
	
	const int piece = GetPieceAt(move.from);
	const int pieceColor = ColorOfPiece(piece);
	const int pieceType = TypeOfPiece(piece);
	const int targetPiece = GetPieceAt(move.to);
	const int targetPieceColor = ColorOfPiece(targetPiece);
	const int targetPieceType = TypeOfPiece(targetPiece);

	switch (piece) {
	case Piece::WhitePawn: SetBitFalse(WhitePawnBits, move.from); SetBitTrue(WhitePawnBits, move.to); break;
	case Piece::BlackPawn: SetBitFalse(BlackPawnBits, move.from); SetBitTrue(BlackPawnBits, move.to); break;
	case Piece::WhiteKnight: SetBitFalse(WhiteKnightBits, move.from); SetBitTrue(WhiteKnightBits, move.to); break;
	case Piece::WhiteBishop: SetBitFalse(WhiteBishopBits, move.from); SetBitTrue(WhiteBishopBits, move.to); break;
	case Piece::WhiteRook: SetBitFalse(WhiteRookBits, move.from); SetBitTrue(WhiteRookBits, move.to); break;
	case Piece::BlackKnight: SetBitFalse(BlackKnightBits, move.from); SetBitTrue(BlackKnightBits, move.to); break;
	case Piece::BlackBishop: SetBitFalse(BlackBishopBits, move.from); SetBitTrue(BlackBishopBits, move.to); break;
	case Piece::BlackRook: SetBitFalse(BlackRookBits, move.from); SetBitTrue(BlackRookBits, move.to); break;
	case Piece::BlackQueen: SetBitFalse(BlackQueenBits, move.from); SetBitTrue(BlackQueenBits, move.to); break;
	case Piece::BlackKing: SetBitFalse(BlackKingBits, move.from); SetBitTrue(BlackKingBits, move.to); break;
	case Piece::WhiteQueen: SetBitFalse(WhiteQueenBits, move.from); SetBitTrue(WhiteQueenBits, move.to); break;
	case Piece::WhiteKing: SetBitFalse(WhiteKingBits, move.from); SetBitTrue(WhiteKingBits, move.to); break;
	}

	switch (targetPiece) {
	case Piece::None: break;
	case Piece::WhitePawn: SetBitFalse(WhitePawnBits, move.to); break;
	case Piece::BlackPawn: SetBitFalse(BlackPawnBits, move.to); break;
	case Piece::WhiteKnight: SetBitFalse(WhiteKnightBits, move.to); break;
	case Piece::WhiteBishop: SetBitFalse(WhiteBishopBits, move.to); break;
	case Piece::WhiteRook: SetBitFalse(WhiteRookBits, move.to); break;
	case Piece::BlackKnight: SetBitFalse(BlackKnightBits, move.to); break;
	case Piece::BlackBishop: SetBitFalse(BlackBishopBits, move.to); break;
	case Piece::BlackRook: SetBitFalse(BlackRookBits, move.to); break;
	case Piece::WhiteQueen: SetBitFalse(WhiteQueenBits, move.to); break;
	case Piece::BlackQueen: SetBitFalse(BlackQueenBits, move.to); break;
	}

	if ((piece == Piece::WhitePawn) && (move.to == EnPassantSquare)) {
		SetBitFalse(BlackPawnBits, EnPassantSquare-8);
	}
	else if ((piece == Piece::BlackPawn) && (move.to == EnPassantSquare)) {
		SetBitFalse(WhitePawnBits, EnPassantSquare+8);
	}

	if (piece == Piece::WhitePawn) {
		switch (move.flag) {
		case MoveFlag::None: break;
		case MoveFlag::PromotionToQueen: SetBitFalse(WhitePawnBits, move.to); SetBitTrue(WhiteQueenBits, move.to); break;
		case MoveFlag::PromotionToKnight: SetBitFalse(WhitePawnBits, move.to); SetBitTrue(WhiteKnightBits, move.to); break;
		case MoveFlag::PromotionToRook: SetBitFalse(WhitePawnBits, move.to); SetBitTrue(WhiteRookBits, move.to); break;
		case MoveFlag::PromotionToBishop: SetBitFalse(WhitePawnBits, move.to); SetBitTrue(WhiteBishopBits, move.to); break;
		}
	}
	else if (piece == Piece::BlackPawn) {
		switch (move.flag) {
		case MoveFlag::None: break;
		case MoveFlag::PromotionToQueen: SetBitFalse(BlackPawnBits, move.to); SetBitTrue(BlackQueenBits, move.to); break;
		case MoveFlag::PromotionToKnight: SetBitFalse(BlackPawnBits, move.to); SetBitTrue(BlackKnightBits, move.to); break;
		case MoveFlag::PromotionToRook: SetBitFalse(BlackPawnBits, move.to); SetBitTrue(BlackRookBits, move.to); break;
		case MoveFlag::PromotionToBishop: SetBitFalse(BlackPawnBits, move.to); SetBitTrue(BlackBishopBits, move.to); break;
		}
	}

	if (targetPiece != 0) HalfmoveClock = 0;
	if (pieceType == PieceType::Pawn) HalfmoveClock = 0;
	if (move.flag == MoveFlag::EnPassantPerformed) HalfmoveClock = 0;

	// 2. Handle castling
	if ((move.flag == MoveFlag::ShortCastle) || (move.flag == MoveFlag::LongCastle)) {
		if ((Turn == Turn::White) && (move.flag == MoveFlag::ShortCastle)) {
			SetBitFalse(WhiteRookBits, Squares::H1);
			SetBitTrue(WhiteRookBits, Squares::F1);
			WhiteRightToShortCastle = false;
			WhiteRightToLongCastle = false;
		}
		else if ((Turn == Turn::White) && (move.flag == MoveFlag::LongCastle)) {
			SetBitFalse(WhiteRookBits, Squares::A1);
			SetBitTrue(WhiteRookBits, Squares::D1);
			WhiteRightToShortCastle = false;
			WhiteRightToLongCastle = false;
		}
		else if ((Turn == Turn::Black) && (move.flag == MoveFlag::ShortCastle)) {
			SetBitFalse(BlackRookBits, Squares::H8);
			SetBitTrue(BlackRookBits, Squares::F8);
			BlackRightToShortCastle = false;
			BlackRightToLongCastle = false;
		}
		else if ((Turn == Turn::Black) && (move.flag == MoveFlag::LongCastle)) {
			SetBitFalse(BlackRookBits, Squares::A8);
			SetBitTrue(BlackRookBits, Squares::D8);
			BlackRightToShortCastle = false;
			BlackRightToLongCastle = false;
		}
	}

	// 3. Update castling rights
	if (piece == Piece::WhiteKing) {
		WhiteRightToShortCastle = false;
		WhiteRightToLongCastle = false;
	}
	if (piece == Piece::BlackKing) {
		BlackRightToShortCastle = false;
		BlackRightToLongCastle = false;
	}
	if ((move.to == Squares::H1) || (move.from == Squares::H1)) WhiteRightToShortCastle = false;
	if ((move.to == Squares::A1) || (move.from == Squares::A1)) WhiteRightToLongCastle = false;
	if ((move.to == Squares::H8) || (move.from == Squares::H8)) BlackRightToShortCastle = false;
	if ((move.to == Squares::A8) || (move.from == Squares::A8)) BlackRightToLongCastle = false;

	// 4. Update en passant
	if (move.flag == MoveFlag::EnPassantPossible) {
		if (Turn == Turn::White) EnPassantSquare = move.to - 8;
		else EnPassantSquare = move.to + 8;
	}
	else {
		EnPassantSquare = -1;
	}

}

void Board::Push(const Move move) {

	if (move.flag != MoveFlag::NullMove) {
		HalfmoveClock += 1;
		TryMove(move);
	}
	else {
		// Handle null moves efficiently
		Turn = !Turn;
		HashValue =	HashInternal();
		return;
	}
	GenerateOccupancy();

	// Increment timers
	Turn = !Turn;
	if (Turn == Turn::White) FullmoveClock += 1;

	// Update threefold repetition list
	if (HalfmoveClock == 0) PreviousHashes.clear();
	HashValue = HashInternal();
	PreviousHashes.push_back(HashValue);

}

// We try to call this function as little as possible
// Pretends to make a move, check its legality and then revert the variables
// It only cares about whether the king will be in check, impossible moves won't be noticed
bool Board::IsLegalMove(const Move m) {
	const uint64_t whitePawnBits = WhitePawnBits;
	const uint64_t whiteKnightBits = WhiteKnightBits;
	const uint64_t whiteBishopBits = WhiteBishopBits;
	const uint64_t whiteRookBits = WhiteRookBits;
	const uint64_t whiteQueenBits = WhiteQueenBits;
	const uint64_t whiteKingBits = WhiteKingBits;
	const uint64_t blackPawnBits = BlackPawnBits;
	const uint64_t blackKnightBits = BlackKnightBits;
	const uint64_t blackBishopBits = BlackBishopBits;
	const uint64_t blackRookBits = BlackRookBits;
	const uint64_t blackQueenBits = BlackQueenBits;
	const uint64_t blackKingBits = BlackKingBits;
	const int enPassantSquare = EnPassantSquare;
	const bool whiteShortCastle = WhiteRightToShortCastle;
	const bool whiteLongCastle = WhiteRightToLongCastle;
	const bool blackShortCastle = BlackRightToShortCastle;
	const bool blackLongCastle = BlackRightToLongCastle;
	const int fullmoveClock = FullmoveClock;
	const int halfmoveClock = HalfmoveClock;
	const bool turn = Turn;

	// Push move
	TryMove(m);

	// Check
	bool inCheck = IsInCheck();

	// Revert
	WhitePawnBits = whitePawnBits;
	WhiteKnightBits = whiteKnightBits;
	WhiteBishopBits = whiteBishopBits;
	WhiteRookBits = whiteRookBits;
	WhiteQueenBits = whiteQueenBits;
	WhiteKingBits = whiteKingBits;
	BlackPawnBits = blackPawnBits;
	BlackKnightBits = blackKnightBits;
	BlackBishopBits = blackBishopBits;
	BlackRookBits = blackRookBits;
	BlackQueenBits = blackQueenBits;
	BlackKingBits = blackKingBits;
	EnPassantSquare = enPassantSquare;
	WhiteRightToShortCastle = whiteShortCastle;
	WhiteRightToLongCastle = whiteLongCastle;
	BlackRightToShortCastle = blackShortCastle;
	BlackRightToLongCastle = blackLongCastle;
	FullmoveClock = fullmoveClock;
	HalfmoveClock = halfmoveClock;
	Turn = turn;

	return !inCheck;
}

const bool Board::IsMoveQuiet(const Move& move) const {
	const uint8_t movedPiece = GetPieceAt(move.from);
	const uint8_t targetPiece = GetPieceAt(move.to);
	if (targetPiece != Piece::None) return false;
	if ((move.flag == MoveFlag::PromotionToQueen) || (move.flag == MoveFlag::PromotionToKnight) || (move.flag == MoveFlag::PromotionToRook) || (move.flag == MoveFlag::PromotionToBishop)) return false;
	if ((movedPiece == Piece::WhitePawn) && (SquareRankArray[move.to] >= 6)) return false;
	if ((movedPiece == Piece::BlackPawn) && (SquareRankArray[move.to] <= 1)) return false;
	if (move.flag == MoveFlag::EnPassantPerformed) return false;
	return true;
}

// Generating moves -------------------------------------------------------------------------------

template <bool side, MoveGen moveGen>
const void Board::GenerateKingMoves(std::vector<Move>& moves, const int home) const {
	const uint8_t friendlyPieceColor = (side == Turn::White) ? PieceColor::White : PieceColor::Black;
	const uint8_t opponentPieceColor = (side == Turn::White) ? PieceColor::Black : PieceColor::White;
	for (const int& l : KingMoves[home]) {
		if (ColorOfPiece(GetPieceAt(l)) == friendlyPieceColor) continue;
		if ((moveGen == MoveGen::All) || (ColorOfPiece(GetPieceAt(l)) == opponentPieceColor)) moves.push_back(Move(home, l));
	}
}

template <bool side, MoveGen moveGen>
const void Board::GenerateKnightMoves(std::vector<Move>& moves, const int home) const {
	const uint8_t friendlyPieceColor = (side == Turn::White) ? PieceColor::White : PieceColor::Black;
	const uint8_t opponentPieceColor = (side == Turn::White) ? PieceColor::Black : PieceColor::White;
	for (const int& l : KnightMoves[home]) {
		if (ColorOfPiece(GetPieceAt(l)) == friendlyPieceColor) continue;
		if ((moveGen == MoveGen::All) || (ColorOfPiece(GetPieceAt(l)) == opponentPieceColor)) moves.push_back(Move(home, l));
	}
}

template <bool side, int pieceType, MoveGen moveGen>
const void Board::GenerateSlidingMoves(std::vector<Move>& moves, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy) const {
	const uint64_t friendlyOccupance = (side == PieceColor::White) ? whiteOccupancy : blackOccupancy;
	const uint64_t opponentOccupance = (side == PieceColor::White) ? blackOccupancy : whiteOccupancy;
	const uint64_t occupancy = whiteOccupancy | blackOccupancy;
	uint64_t map = 0;

	switch (pieceType) {
	case PieceType::Rook:
		map = GetRookAttacks(home, occupancy) & ~friendlyOccupance;
		break;
	case PieceType::Bishop:
		map = GetBishopAttacks(home, occupancy) & ~friendlyOccupance;
		break;
	case PieceType::Queen:
		map = (GetQueenAttacks(home, occupancy)) & ~friendlyOccupance;
		break;
	}

	if (moveGen == MoveGen::Noisy) map &= opponentOccupance;
	if (map == 0) return;

	while (map != 0) {
		const int sq = Popsquare(map);
		moves.push_back(Move(home, sq));
	}
}

template <bool side, MoveGen moveGen>
const void Board::GeneratePawnMoves(std::vector<Move>& moves, const int home) const {
	const int file = GetSquareFile(home);
	const int rank = GetSquareRank(home);
	int target;

	if (side == Turn::White) {
		// 1. Can move forward? + check promotions
		target = home + 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 7) {
				if ((moveGen == MoveGen::All) || (SquareRankArray[target] == 6)) moves.push_back(Move(home, target));
			} else { // Promote
				moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
			}
		}

		// 2. Can move up left (can capture?) + check promotions + check en passant
		target = home + 7;
		if ((file != 0) && ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target==EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0))) {
			if (GetSquareRank(target) != 7) {
				Move m = Move(home, target, target == EnPassantSquare ? MoveFlag::EnPassantPerformed : 0);
				moves.push_back(m);
			} else { // Promote
				moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
			}
		}

		// 3. Can move up right (can capture?) + check promotions + check en passant
		target = home + 9;
		if (file != 7) {
			if ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0)) {
				if (GetSquareRank(target) != 7) {
					Move m = Move(home, target, target == EnPassantSquare ? MoveFlag::EnPassantPerformed : 0);
					moves.push_back(m);
				}
				else { // Promote
					moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
					if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
					if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
					if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 1) {
			bool free1 = GetPieceAt(home + 8) == 0;
			bool free2 = GetPieceAt(home + 16) == 0;
			if (free1 && free2) {
				if (moveGen == MoveGen::All) moves.push_back(Move(home, home+16, MoveFlag::EnPassantPossible));
			}
		}
	}

	if (side == Turn::Black) {
		// 1. Can move forward? + check promotions
		target = home - 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 0) {
				if ((moveGen == MoveGen::All) || (SquareRankArray[target] == 1)) moves.push_back(Move(home, target));
			} else { // Promote
				moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
			}
		}

		// 2. Can move down right (can capture?) + check promotions + check en passant
		target = home - 7;
		if ((file != 7) && ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0))) {
			if (GetSquareRank(target) != 0) {
				Move m = Move(home, target, target == EnPassantSquare ? MoveFlag::EnPassantPerformed : 0);
				moves.push_back(m);
			} else { // Promote
				moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
			}
		}

		// 3. Can move down left (can capture?) + check promotions + check en passant
		target = home - 9;
		if (file != 0) {
			if ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0)) {
				if (GetSquareRank(target) != 0) {
					Move m = Move(home, target, target == EnPassantSquare ? MoveFlag::EnPassantPerformed : 0);
					moves.push_back(m);
				}
				else { // Promote
					moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
					if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
					if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
					if (moveGen == MoveGen::All) moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 6) {
			bool free1 = GetPieceAt(home - 8) == 0;
			bool free2 = GetPieceAt(home - 16) == 0;
			if (free1 && free2) {
				if (moveGen == MoveGen::All) moves.push_back(Move(home, home - 16, MoveFlag::EnPassantPossible));
			}
		}
	}
}

template <bool side>
const void Board::GenerateCastlingMoves(std::vector<Move>& moves) const {
	if ((Turn == Turn::White) && (WhiteRightToShortCastle)) {
		const bool empty_f1 = GetPieceAt(Squares::F1) == 0;
		const bool empty_g1 = GetPieceAt(Squares::G1) == 0;
		if (empty_f1 && empty_g1) {
			const bool safe_e1 = !IsSquareAttacked<Turn::Black>(Squares::E1);
			const bool safe_f1 = !IsSquareAttacked<Turn::Black>(Squares::F1);
			const bool safe_g1 = !IsSquareAttacked<Turn::Black>(Squares::G1);
			if (safe_e1 && safe_f1 && safe_g1) {
				moves.push_back(Move(Squares::E1, Squares::G1, MoveFlag::ShortCastle));
			}
		}
	}
	if ((Turn == Turn::White) && (WhiteRightToLongCastle)) {
		const bool empty_b1 = GetPieceAt(Squares::B1) == 0;
		const bool empty_c1 = GetPieceAt(Squares::C1) == 0;
		const bool empty_d1 = GetPieceAt(Squares::D1) == 0;
		if (empty_b1 && empty_c1 && empty_d1) {
			const bool safe_c1 = !IsSquareAttacked<Turn::Black>(Squares::C1);
			const bool safe_d1 = !IsSquareAttacked<Turn::Black>(Squares::D1);
			const bool safe_e1 = !IsSquareAttacked<Turn::Black>(Squares::E1);
			if (safe_c1 && safe_d1 && safe_e1) {
				moves.push_back(Move(Squares::E1, Squares::C1, MoveFlag::LongCastle));
			}
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToShortCastle)) {
		const bool empty_f8 = GetPieceAt(Squares::F8) == 0;
		const bool empty_g8 = GetPieceAt(Squares::G8) == 0;
		if (empty_f8 && empty_g8) {
			const bool safe_e8 = !IsSquareAttacked<Turn::White>(Squares::E8);
			const bool safe_f8 = !IsSquareAttacked<Turn::White>(Squares::F8);
			const bool safe_g8 = !IsSquareAttacked<Turn::White>(Squares::G8);
			if (safe_e8 && safe_f8 && safe_g8) {
				moves.push_back(Move(Squares::E8, Squares::G8, MoveFlag::ShortCastle));
			}
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToLongCastle)) {
		const bool empty_b8 = GetPieceAt(Squares::B8) == 0;
		const bool empty_c8 = GetPieceAt(Squares::C8) == 0;
		const bool empty_d8 = GetPieceAt(Squares::D8) == 0;
		if (empty_b8 & empty_c8 & empty_d8) {
			const bool safe_c8 = !IsSquareAttacked<Turn::White>(Squares::C8);
			const bool safe_d8 = !IsSquareAttacked<Turn::White>(Squares::D8);
			const bool safe_e8 = !IsSquareAttacked<Turn::White>(Squares::E8);
			if (safe_c8 && safe_d8 && safe_e8) {
				moves.push_back(Move(Squares::E8, Squares::C8, MoveFlag::LongCastle));
			}
		}
	}
}

const void Board::GenerateMoves(std::vector<Move>& moves, const MoveGen moveGen, const Legality legality) {

	if (legality == Legality::Pseudolegal) {
		if (moveGen == MoveGen::All) {
			if (Turn == Turn::White) GeneratePseudolegalMoves<Turn::White, MoveGen::All>(moves);
			else GeneratePseudolegalMoves<Turn::Black, MoveGen::All>(moves);
		}
		else if (moveGen == MoveGen::Noisy) {
			if (Turn == Turn::White) GeneratePseudolegalMoves<Turn::White, MoveGen::Noisy>(moves);
			else GeneratePseudolegalMoves<Turn::Black, MoveGen::Noisy>(moves);
		}
	}
	else {
		std::vector<Move> legalMoves;
		if (moveGen == MoveGen::All) {
			if (Turn == Turn::White) GeneratePseudolegalMoves<Turn::White, MoveGen::All>(legalMoves);
			else GeneratePseudolegalMoves<Turn::Black, MoveGen::All>(legalMoves);
		}
		else if (moveGen == MoveGen::Noisy) {
			if (Turn == Turn::White) GeneratePseudolegalMoves<Turn::White, MoveGen::Noisy>(legalMoves);
			else GeneratePseudolegalMoves<Turn::Black, MoveGen::Noisy>(legalMoves);
		}
		for (const Move& m : legalMoves) {
			if (IsLegalMove(m)) moves.push_back(m);
		}
	}
}

template <bool side, MoveGen moveGen>
const void Board::GeneratePseudolegalMoves(std::vector<Move>& moves) const {

	/*
	const uint64_t whiteOccupancy = GetOccupancy(PieceColor::White);
	const uint64_t blackOccupancy = GetOccupancy(PieceColor::Black);
	uint64_t friendlyOccupancy = (Turn == Turn::White) ? whiteOccupancy : blackOccupancy;
	
	uint64_t occupancy = (side == Turn::White) ? WhitePawnBits : BlackPawnBits;
	while (occupancy != 0) {
		const int sq = Popsquare(occupancy);
		GeneratePawnMoves<side, moveGen>(moves, sq);
	}

	occupancy = (side == Turn::White) ? WhiteKnightBits : BlackKnightBits;
	while (occupancy != 0) {
		const int sq = Popsquare(occupancy);
		GenerateKnightMoves<side, moveGen>(moves, sq);
	}

	occupancy = (side == Turn::White) ? WhiteBishopBits : BlackBishopBits;
	while (occupancy != 0) {
		const int sq = Popsquare(occupancy);
		GenerateSlidingMoves<side, PieceType::Bishop, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
	}

	occupancy = (side == Turn::White) ? WhiteRookBits : BlackRookBits;
	while (occupancy != 0) {
		const int sq = Popsquare(occupancy);
		GenerateSlidingMoves<side, PieceType::Rook, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
	}

	occupancy = (side == Turn::White) ? WhiteQueenBits : BlackQueenBits;
	while (occupancy != 0) {
		const int sq = Popsquare(occupancy);
		GenerateSlidingMoves<side, PieceType::Queen, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
	}

	occupancy = (side == Turn::White) ? WhiteKingBits : BlackKingBits;
	const int sq = Popsquare(occupancy);
	GenerateKingMoves<side, moveGen>(moves, sq);
	if (moveGen == MoveGen::All) GenerateCastlingMoves<side>(moves);
	*/


	const uint64_t whiteOccupancy = GetOccupancy(PieceColor::White);
	const uint64_t blackOccupancy = GetOccupancy(PieceColor::Black);
	uint64_t friendlyOccupancy = (Turn == Turn::White) ? whiteOccupancy : blackOccupancy;
	const bool quiescenceOnly = (moveGen == MoveGen::Noisy) ? true : false;

	while (friendlyOccupancy != 0) {
		const int sq = Popsquare(friendlyOccupancy);
		const int type = TypeOfPiece(GetPieceAt(sq));

		if (type == PieceType::Pawn) GeneratePawnMoves<side, moveGen>(moves, sq);
		else if (type == PieceType::Knight) GenerateKnightMoves<side, moveGen>(moves, sq);
		else if (type == PieceType::Bishop) GenerateSlidingMoves<side, PieceType::Bishop, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
		else if (type == PieceType::Rook) GenerateSlidingMoves<side, PieceType::Rook, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
		else if (type == PieceType::Queen) GenerateSlidingMoves<side, PieceType::Queen, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
		else if (type == PieceType::King) {
			GenerateKingMoves<side, moveGen>(moves, sq);
			if (moveGen == MoveGen::All) GenerateCastlingMoves<side>(moves);
		}
	}
}

// Generating attack maps -------------------------------------------------------------------------

const uint64_t Board::CalculateAttackedSquares(const uint8_t colorOfPieces) const {
	uint64_t squares = 0ULL;
	uint64_t parallelSliders = 0;
	uint64_t diagonalSliders = 0;
	uint64_t friendlyPieces = 0;
	uint64_t opponentPieces = 0;

	if (colorOfPieces == PieceColor::White) {
		parallelSliders = WhiteRookBits | WhiteQueenBits;
		diagonalSliders = WhiteBishopBits | WhiteQueenBits;
		friendlyPieces = GetOccupancy(PieceColor::White);
		opponentPieces = GetOccupancy(PieceColor::Black);
	}
	else {
		parallelSliders = BlackRookBits | BlackQueenBits;
		diagonalSliders = BlackBishopBits | BlackQueenBits;
		friendlyPieces = GetOccupancy(PieceColor::Black);
		opponentPieces = GetOccupancy(PieceColor::White);
	}

	// Sliding piece attack generation
	uint64_t occ = GetOccupancy();
	uint64_t parallelBits = colorOfPieces == PieceColor::White ? WhiteRookBits | WhiteQueenBits : BlackRookBits | BlackQueenBits;
	while (parallelBits != 0) {
		const uint8_t sq = Popsquare(parallelBits);
		squares |= GetRookAttacks(sq, occ);
	}
	uint64_t diagonalBits = colorOfPieces == PieceColor::White ? WhiteBishopBits | WhiteQueenBits : BlackBishopBits | BlackQueenBits;
	while (diagonalBits != 0) {
		const uint8_t sq = Popsquare(diagonalBits);
		squares |= GetBishopAttacks(sq, occ);
	}

	// King attack gen
	uint8_t kingSquare = 63 - Lzcount(colorOfPieces == PieceColor::White ? WhiteKingBits : BlackKingBits);
	uint64_t fill = KingMoveBits[kingSquare];
	squares |= fill;

	// Knight attack gen
	uint64_t knightBits = colorOfPieces == PieceColor::White ? WhiteKnightBits : BlackKnightBits;
	fill = 0;
	while (knightBits != 0) {
		const uint8_t sq = Popsquare(knightBits);
		fill |= GenerateKnightAttacks(sq);
	}
	squares |= fill;

	// Pawn attack map generation
	if (colorOfPieces == PieceColor::White) {
		squares |= (WhitePawnBits & ~Bitboards::FileA) << 7;
		squares |= (WhitePawnBits & ~Bitboards::FileH) << 9;
		if (EnPassantSquare != -1) {
			uint64_t encodedEP = 0;
			SetBitTrue(encodedEP, EnPassantSquare);
			squares |= ((WhitePawnBits & ~Bitboards::FileA) >> 1) & encodedEP;
			squares |= ((WhitePawnBits & ~Bitboards::FileH) << 1) & encodedEP;
		}

	}
	else {
		squares |= (BlackPawnBits & ~Bitboards::FileA) >> 9;
		squares |= (BlackPawnBits & ~Bitboards::FileH) >> 7;
		if (EnPassantSquare != -1) {
			uint64_t encodedEP = 0;
			SetBitTrue(encodedEP, EnPassantSquare);
			squares |= ((BlackPawnBits & ~Bitboards::FileA) >> 1) & encodedEP;
			squares |= ((BlackPawnBits & ~Bitboards::FileH) << 1) & encodedEP;
		}
	}

	return squares & ~friendlyPieces; // the second part shouldn't be necessary 
}

const uint64_t Board::GenerateKnightAttacks(const int from) const {
	return KnightMoveBits[from];
}

const uint64_t Board::GenerateKingAttacks(const int from) const {
	return KingMoveBits[from];
}

template <bool attackingSide>
bool Board::IsSquareAttacked(const uint8_t square) const {
	uint64_t occupancy = GetOccupancy();

	if (attackingSide == Turn::White) {
		// Attacked by a knight?
		if (KnightMoveBits[square] & WhiteKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & WhiteKingBits) return true;
		// Attacked by a pawn?
		if (SquareBits[square] & ((WhitePawnBits & ~Bitboards::FileA) << 7)) return true;
		if (SquareBits[square] & ((WhitePawnBits & ~Bitboards::FileH) << 9)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (WhiteRookBits | WhiteQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (WhiteBishopBits | WhiteQueenBits)) return true;
		// Okay
		return false;
	}
	else {
		// Attacked by a knight?
		if (KnightMoveBits[square] & BlackKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & BlackKingBits) return true;
		// Attacked by a pawn?
		if (SquareBits[square] & ((BlackPawnBits & ~Bitboards::FileA) >> 9)) return true;
		if (SquareBits[square] & ((BlackPawnBits & ~Bitboards::FileH) >> 7)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (BlackRookBits | BlackQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (BlackBishopBits | BlackQueenBits)) return true;
		// Okay
		return false;
	}
}

const uint64_t Board::GetAttackersOfSquare(const uint8_t square) const {
	uint64_t occupancy = GetOccupancy();
	uint64_t attackers = 0;

	// Pawns
	if (SquareBits[square] & ((WhitePawnBits & ~Bitboards::FileA) << 7)) SetBitTrue(attackers, square - 7);
	if (SquareBits[square] & ((WhitePawnBits & ~Bitboards::FileH) << 9)) SetBitTrue(attackers, square - 9);
	if (SquareBits[square] & ((BlackPawnBits & ~Bitboards::FileA) >> 9)) SetBitTrue(attackers, square + 9);
	if (SquareBits[square] & ((BlackPawnBits & ~Bitboards::FileH) >> 7)) SetBitTrue(attackers, square + 7);

	// Knights
	uint64_t bits = WhiteKnightBits | BlackKnightBits;
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		if (KnightMoveBits[sq] & SquareBits[square]) SetBitTrue(attackers, sq);
	}

	// Bishops & queens
	bits = WhiteBishopBits | WhiteQueenBits | BlackBishopBits | BlackQueenBits;
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		if (GetBishopAttacks(sq, occupancy) & SquareBits[square]) SetBitTrue(attackers, sq);
	}

	// Rooks & queens
	bits = WhiteRookBits | WhiteQueenBits | BlackRookBits | BlackQueenBits;
	while (bits) {
		const uint8_t sq = Popsquare(bits);
		if (GetRookAttacks(sq, occupancy) & SquareBits[square]) SetBitTrue(attackers, sq);
	}

	// Kings
	uint8_t sq = GetKingSquare<Turn::White>();
	if (KingMoveBits[sq] & SquareBits[square]) SetBitTrue(attackers, sq);
	sq = GetKingSquare<Turn::Black>();
	if (KingMoveBits[sq] & SquareBits[square]) SetBitTrue(attackers, sq);

	return attackers;
}

// Other ------------------------------------------------------------------------------------------

const bool Board::AreThereLegalMoves() {
	bool hasMoves = false;
	const int myColor = TurnToPieceColor(Turn);
	const uint64_t whiteOccupancy = GetOccupancy(PieceColor::White);
	const uint64_t blackOccupancy = GetOccupancy(PieceColor::Black);

	std::vector<Move> moves;
	uint64_t occupancy = (Turn == Turn::White) ? whiteOccupancy : blackOccupancy;
	while (occupancy != 0) {
		int i = 63 - Lzcount(occupancy);
		SetBitFalse(occupancy, i);
		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		if (type == Piece::WhitePawn) GeneratePawnMoves<Turn::White, MoveGen::All>(moves, i);
		if (type == Piece::BlackPawn) GeneratePawnMoves<Turn::Black, MoveGen::All>(moves, i);
		else if (type == Piece::WhiteKnight) GenerateKnightMoves<Turn::White, MoveGen::All>(moves, i);
		else if (type == Piece::BlackKnight) GenerateKnightMoves<Turn::Black, MoveGen::All>(moves, i);
		else if (type == Piece::WhiteBishop) GenerateSlidingMoves<Turn::White, PieceType::Bishop, MoveGen::All>(moves, i, whiteOccupancy, blackOccupancy);
		else if (type == Piece::BlackBishop) GenerateSlidingMoves<Turn::Black, PieceType::Bishop, MoveGen::All>(moves, i, whiteOccupancy, blackOccupancy);
		else if (type == Piece::WhiteRook) GenerateSlidingMoves<Turn::White, PieceType::Rook, MoveGen::All>(moves, i, whiteOccupancy, blackOccupancy);
		else if (type == Piece::BlackRook) GenerateSlidingMoves<Turn::Black, PieceType::Rook, MoveGen::All>(moves, i, whiteOccupancy, blackOccupancy);
		else if (type == Piece::WhiteQueen) GenerateSlidingMoves<Turn::White, PieceType::Queen, MoveGen::All>(moves, i, whiteOccupancy, blackOccupancy);
		else if (type == Piece::BlackQueen) GenerateSlidingMoves<Turn::Black, PieceType::Queen, MoveGen::All>(moves, i, whiteOccupancy, blackOccupancy);
		else if (type == Piece::WhiteKing) {
			GenerateKingMoves<Turn::White, MoveGen::All>(moves, i);
			GenerateCastlingMoves<Turn::White>(moves);
		}
		else if (type == Piece::BlackKing) {
			GenerateKingMoves<Turn::Black, MoveGen::All>(moves, i);
			GenerateCastlingMoves<Turn::Black>(moves);
		}

		if (moves.size() != 0) {
			for (Move m : moves) {
				if (IsLegalMove(m)) {
					moves.clear();
					hasMoves = true;
					break;
				}
			}
			moves.clear();
		}
		if (hasMoves) break;
	}
	return hasMoves;
}

const bool Board::IsDraw() const {

	// Threefold repetitions
	const int64_t stateCount = std::count(PreviousHashes.begin(), PreviousHashes.end(), HashValue);
	if (stateCount >= 3) return true;

	// Insufficient material check
	// I think this neglects some cases when pawns can't move
	const int pieceCount = Popcount(GetOccupancy());
	if (pieceCount <= 4) {
		const int whitePawnCount = Popcount(WhitePawnBits);
		const int whiteKnightCount = Popcount(WhiteKnightBits);
		const int whiteBishopCount = Popcount(WhiteBishopBits);
		const int whiteRookCount = Popcount(WhiteRookBits);
		const int whiteQueenCount = Popcount(WhiteQueenBits);
		const int blackPawnCount = Popcount(BlackPawnBits);
		const int blackKnightCount = Popcount(BlackKnightBits);
		const int blackBishopCount = Popcount(BlackBishopBits);
		const int blackRookCount = Popcount(BlackRookBits);
		const int blackQueenCount = Popcount(BlackQueenBits);
		const int whitePieceCount = whitePawnCount + whiteKnightCount + whiteBishopCount + whiteRookCount + whiteQueenCount;
		const int blackPieceCount = blackPawnCount + blackKnightCount + blackBishopCount + blackRookCount + blackQueenCount;
		const int whiteLightBishopCount = Popcount(WhiteBishopBits & Bitboards::LightSquares);
		const int whiteDarkBishopCount = Popcount(WhiteBishopBits & Bitboards::DarkSquares);
		const int blackLightBishopCount = Popcount(BlackBishopBits & Bitboards::LightSquares);
		const int blackDarkBishopCount = Popcount(BlackBishopBits & Bitboards::DarkSquares);

		if (whitePieceCount == 0 && blackPieceCount == 0) return true;
		if (whitePieceCount == 1 && whiteKnightCount == 1 && blackPieceCount == 0) return true;
		if (blackPieceCount == 1 && blackKnightCount == 1 && whitePieceCount == 0) return true;
		if (whitePieceCount == 1 && whiteBishopCount == 1 && blackPieceCount == 0) return true;
		if (blackPieceCount == 1 && blackBishopCount == 1 && whitePieceCount == 0) return true;
		if (whitePieceCount == 1 && whiteLightBishopCount == 1 && blackPieceCount == 1 && blackLightBishopCount == 1) return true;
		if (whitePieceCount == 1 && whiteDarkBishopCount == 1 && blackPieceCount == 1 && blackDarkBishopCount == 1) return true;
	}

	// Fifty moves without progress
	if (HalfmoveClock >= 100) return true;

	// Return false otherwise
	return false;
}

const GameState Board::GetGameState() {

	// Check checkmates & stalemates
	if (!AreThereLegalMoves()) {
		if (IsInCheck()) {
			if (Turn == Turn::Black) return GameState::WhiteVictory;
			else return GameState::BlackVictory;
		}
		else {
			return GameState::Draw; // Stalemate
		}
	}

	// Check other types of draws
	if (IsDraw()) return GameState::Draw;
	else return GameState::Playing;
}

const std::string Board::GetFEN() const {
	std::string result;
	for (int r = 7; r >= 0; r--) {
		int spaces = 0;
		for (int f = 0; f < 8; f++) {
			int piece = GetPieceAt(Square(r, f));
			if (piece == 0) {
				spaces += 1;
			}
			else {
				if (spaces != 0) result += std::to_string(spaces);
				result += PieceChars[piece];
				spaces = 0;
			}
		}
		if (spaces != 0) result += std::to_string(spaces);
		if (r != 0) result += '/';
	}

	if (Turn == Turn::White) result += " w ";
	else if (Turn == Turn::Black) result += " b ";

	bool castlingPossible = false;
	if (WhiteRightToShortCastle) { result += 'K'; castlingPossible = true; }
	if (WhiteRightToLongCastle) { result += 'Q'; castlingPossible = true; }
	if (BlackRightToShortCastle) { result += 'k'; castlingPossible = true; }
	if (BlackRightToLongCastle) { result += 'q'; castlingPossible = true; }
	if (!castlingPossible) result += '-';
	result += ' ';

	bool enPassantPossible = false;
	if ((EnPassantSquare != -1) && (Turn == Turn::White)) {
		const bool fromRight = (((WhitePawnBits & ~Bitboards::FileA) << 7) & SquareBits[EnPassantSquare]);
		const bool fromLeft = (((WhitePawnBits & ~Bitboards::FileH) << 9) & SquareBits[EnPassantSquare]);
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if ((EnPassantSquare != -1) && (Turn == Turn::Black)) {
		const bool fromRight = (((BlackPawnBits & ~Bitboards::FileA) >> 9) & SquareBits[EnPassantSquare]);
		const bool fromLeft = (((BlackPawnBits & ~Bitboards::FileH) >> 7) & SquareBits[EnPassantSquare]);
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if (enPassantPossible) result += SquareStrings[EnPassantSquare];
	else result += '-';

	result += ' ' + std::to_string(HalfmoveClock) + ' ' + std::to_string(FullmoveClock);
	return result;
}

const int Board::GetPlys() const {
	return (FullmoveClock - 1) * 2 + (Turn == Turn::White ? 0 : 1);
}

template <bool side>
const uint8_t Board::GetKingSquare() const {
	if (side == Turn::White) return 63 - Lzcount(WhiteKingBits);	
	else return 63 - Lzcount(BlackKingBits);
}

const bool Board::IsInCheck() const {
	if (Turn == Turn::White) return IsSquareAttacked<Turn::Black>(63 - Lzcount(WhiteKingBits));
	else return IsSquareAttacked<Turn::White>(63 - Lzcount(BlackKingBits));
}