﻿#include "Board.h"

// Constructors and related -----------------------------------------------------------------------

Board::Board(const std::string& fen) {
	std::vector<std::string> parts = Split(fen);
	int place = 56;

	for (const char &f : parts[0]) {
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

	if (parts[1] == "w") Turn = Side::White;
	else Turn = Side::Black;

	for (const char &f : parts[2]) {
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
	HashValue = b.HashValue;

	OccupancyInts = b.OccupancyInts;
	PreviousHashes.reserve(b.PreviousHashes.size() + 1);
	PreviousHashes = b.PreviousHashes;
}

// Generating board hash --------------------------------------------------------------------------

uint64_t Board::HashInternal() {
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

	int sq = LsbSquare(WhiteKingBits);
	hash ^= Zobrist[64 * 11 + sq];

	sq = LsbSquare(BlackKingBits);
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
			if (Turn == Side::White) hasPawn = CheckBit(WhitePawnBits, EnPassantSquare - 7);
			else hasPawn = CheckBit(BlackPawnBits, EnPassantSquare + 9);
		}
		if ((GetSquareFile(EnPassantSquare) != 7) && !hasPawn) {
			if (Turn == Side::White) hasPawn = CheckBit(WhitePawnBits, EnPassantSquare - 9);
			else hasPawn = CheckBit(BlackPawnBits, EnPassantSquare + 7);
		}
		if (hasPawn) hash ^= Zobrist[772 + GetSquareFile(EnPassantSquare)];
	}

	// Turn
	if (Turn == Side::White) hash ^= Zobrist[780];

	return hash;
}

uint64_t Board::Hash() const {
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

	int sq = LsbSquare(WhiteKingBits);
	OccupancyInts[sq] = Piece::WhiteKing;

	sq = LsbSquare(BlackKingBits);
	OccupancyInts[sq] = Piece::BlackKing;
}

uint64_t Board::GetOccupancy() const {
	return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits
		| BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

uint64_t Board::GetOccupancy(const bool side) const {
	if (side == Side::White) return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
	else return BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

uint8_t Board::GetPieceAt(const uint8_t place) const {
	return OccupancyInts[place];
}

// Making moves -----------------------------------------------------------------------------------

// Converts the uci move input to the engine's own representation, and then plays it
bool Board::PushUci(const std::string& ucistr) {
	const int sq1 = SquareToNum(ucistr.substr(0, 2));
	const int sq2 = SquareToNum(ucistr.substr(2, 2));
	const char extra = ucistr[4];
	Move move = Move(sq1, sq2);
	const int piece = GetPieceAt(sq1);
	const int capturedPiece = GetPieceAt(sq2);

	// Promotions
	switch (extra) {
	case 'q': move.flag = MoveFlag::PromotionToQueen; break;
	case 'r': move.flag = MoveFlag::PromotionToRook; break;
	case 'b': move.flag = MoveFlag::PromotionToBishop; break;
	case 'n': move.flag = MoveFlag::PromotionToKnight; break;
	}

	// Castling
	if ((piece == Piece::WhiteKing) && (sq1 == 4) && (sq2 == 2)) move.flag = MoveFlag::LongCastle;
	else if ((piece == Piece::WhiteKing) && (sq1 == 4) && (sq2 == 6)) move.flag = MoveFlag::ShortCastle;
	else if ((piece == Piece::BlackKing) && (sq1 == 60) && (sq2 == 58)) move.flag = MoveFlag::LongCastle;
	else if ((piece == Piece::BlackKing) && (sq1 == 60) && (sq2 == 62)) move.flag = MoveFlag::ShortCastle;

	// En passant possibility
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		const int f1 = sq1 / 8;
		const int f2 = sq2 / 8;
		if (abs(f2 - f1) > 1) move.flag = MoveFlag::EnPassantPossible;
	}

	// En passant performed
	if (TypeOfPiece(piece) == PieceType::Pawn) {
		if ((TypeOfPiece(capturedPiece) == 0) && (GetSquareFile(sq1) != GetSquareFile(sq2))) {
			move.flag = MoveFlag::EnPassantPerformed;
		}
	}

	// Generate the list of valid moves
	MoveList legalMoves{};
	GenerateMoves(legalMoves, MoveGen::All, Legality::Legal);
	bool valid = false;
	for (const auto& m : legalMoves) {
		if ((m.move.to == move.to) && (m.move.from == move.from) && (m.move.flag == move.flag)) {
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
	const int pieceType = TypeOfPiece(piece);
	const int targetPiece = GetPieceAt(move.to);

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
		if ((Turn == Side::White) && (move.flag == MoveFlag::ShortCastle)) {
			SetBitFalse(WhiteRookBits, Squares::H1);
			SetBitTrue(WhiteRookBits, Squares::F1);
			WhiteRightToShortCastle = false;
			WhiteRightToLongCastle = false;
		}
		else if ((Turn == Side::White) && (move.flag == MoveFlag::LongCastle)) {
			SetBitFalse(WhiteRookBits, Squares::A1);
			SetBitTrue(WhiteRookBits, Squares::D1);
			WhiteRightToShortCastle = false;
			WhiteRightToLongCastle = false;
		}
		else if ((Turn == Side::Black) && (move.flag == MoveFlag::ShortCastle)) {
			SetBitFalse(BlackRookBits, Squares::H8);
			SetBitTrue(BlackRookBits, Squares::F8);
			BlackRightToShortCastle = false;
			BlackRightToLongCastle = false;
		}
		else if ((Turn == Side::Black) && (move.flag == MoveFlag::LongCastle)) {
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
		if (Turn == Side::White) EnPassantSquare = move.to - 8;
		else EnPassantSquare = move.to + 8;
	}
	else {
		EnPassantSquare = -1;
	}

}

void Board::Push(const Move& move) {

	if (move.flag != MoveFlag::NullMove) {
		HalfmoveClock += 1;
		TryMove(move);
	}
	else {
		// Handle null moves efficiently
		Turn = !Turn;
		EnPassantSquare = -1;
		HashValue = HashInternal();
		return;
	}
	GenerateOccupancy();

	// Increment timers
	Turn = !Turn;
	if (Turn == Side::White) FullmoveClock += 1;

	// Update threefold repetition list
	if (HalfmoveClock == 0) PreviousHashes.clear();
	HashValue = HashInternal();
	PreviousHashes.push_back(HashValue);

}

// We try to call this function as little as possible
// Pretends to make a move, check its legality and then revert the variables
// It only cares about whether the king will be in check, impossible moves won't be noticed
bool Board::IsLegalMove(const Move& m) {
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
	const int8_t enPassantSquare = EnPassantSquare;
	const bool whiteShortCastle = WhiteRightToShortCastle;
	const bool whiteLongCastle = WhiteRightToLongCastle;
	const bool blackShortCastle = BlackRightToShortCastle;
	const bool blackLongCastle = BlackRightToLongCastle;
	const uint16_t fullmoveClock = FullmoveClock;
	const uint8_t halfmoveClock = HalfmoveClock;
	const bool turn = Turn;

	// Push move
	TryMove(m);

	// Check
	const bool inCheck = IsInCheck();

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

bool Board::IsMoveQuiet(const Move& move) const {
	const uint8_t movedPiece = GetPieceAt(move.from);
	const uint8_t targetPiece = GetPieceAt(move.to);
	if (targetPiece != Piece::None) return false;
	if (move.flag == MoveFlag::PromotionToQueen) return false;
	if (move.flag == MoveFlag::EnPassantPerformed) return false;
	return true;
}

// Generating moves -------------------------------------------------------------------------------

template <bool side, MoveGen moveGen>
void Board::GenerateKingMoves(MoveList& moves, const int home) const {
	constexpr uint8_t friendlyPieceColor = (side == Side::White) ? PieceColor::White : PieceColor::Black;
	constexpr uint8_t opponentPieceColor = (side == Side::White) ? PieceColor::Black : PieceColor::White;
	uint64_t bits = KingMoveBits[home];
	while (bits) {
		const uint8_t l = Popsquare(bits);
		if (ColorOfPiece(GetPieceAt(l)) == friendlyPieceColor) continue;
		if ((moveGen == MoveGen::All) || (ColorOfPiece(GetPieceAt(l)) == opponentPieceColor)) moves.pushUnscored(Move(home, l));
	}
}

template <bool side, MoveGen moveGen>
void Board::GenerateKnightMoves(MoveList& moves, const int home) const {
	constexpr uint8_t friendlyPieceColor = (side == Side::White) ? PieceColor::White : PieceColor::Black;
	constexpr uint8_t opponentPieceColor = (side == Side::White) ? PieceColor::Black : PieceColor::White;
	uint64_t bits = KnightMoveBits[home];
	while (bits) {
		const uint8_t l = Popsquare(bits);
		if (ColorOfPiece(GetPieceAt(l)) == friendlyPieceColor) continue;
		if ((moveGen == MoveGen::All) || (ColorOfPiece(GetPieceAt(l)) == opponentPieceColor)) moves.pushUnscored(Move(home, l));
	}
}

template <bool side, int pieceType, MoveGen moveGen>
void Board::GenerateSlidingMoves(MoveList& moves, const int home, const uint64_t whiteOccupancy, const uint64_t blackOccupancy) const {
	const uint64_t friendlyOccupance = (side == PieceColor::White) ? whiteOccupancy : blackOccupancy;
	const uint64_t opponentOccupance = (side == PieceColor::White) ? blackOccupancy : whiteOccupancy;
	const uint64_t occupancy = whiteOccupancy | blackOccupancy;
	uint64_t map = 0;

	if constexpr (pieceType == PieceType::Rook) map = GetRookAttacks(home, occupancy) & ~friendlyOccupance;
	if constexpr (pieceType == PieceType::Bishop) map = GetBishopAttacks(home, occupancy) & ~friendlyOccupance;
	if constexpr (pieceType == PieceType::Queen) map = GetQueenAttacks(home, occupancy) & ~friendlyOccupance;

	if constexpr (moveGen == MoveGen::Noisy) map &= opponentOccupance;
	if (map == 0) return;

	while (map != 0) {
		const int sq = Popsquare(map);
		moves.pushUnscored(Move(home, sq));
	}
}

template <bool side, MoveGen moveGen>
void Board::GeneratePawnMoves(MoveList& moves, const int home) const {

	constexpr int promotionRank = (side == Side::White) ? 7 : 0;
	constexpr int doublePushRank = (side == Side::White) ? 1 : 6;
	constexpr int forwardDelta = (side == Side::White) ? 8 : -8;
	constexpr int doublePushDelta = (side == Side::White) ? 16 : -16;
	constexpr uint8_t opponentPieceColor = SideToPieceColor(!side);

	// These are like that to avoid bench changing, will be changed to be more intuitive later
	constexpr int capture1Delta = (side == Side::White) ? 7 : -7;
	constexpr int capture2Delta = (side == Side::White) ? 9 : -9;
	constexpr int capture1WrongFile = (side == Side::White) ? 0 : 7;
	constexpr int capture2WrongFile = (side == Side::White) ? 7 : 0;

	const int file = GetSquareFile(home);
	const int rank = GetSquareRank(home);
	int target;

	// 1. Handle moving forward + check promotions
	target = home + forwardDelta;
	if (GetPieceAt(target) == Piece::None) {
		if (GetSquareRank(target) != promotionRank) {
			if constexpr (moveGen == MoveGen::All) moves.pushUnscored(Move(home, target));
		}
		else { // Promote
			moves.pushUnscored(Move(home, target, MoveFlag::PromotionToQueen));
			if constexpr (moveGen == MoveGen::All) {
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToRook));
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToBishop));
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToKnight));
			}
		}
	}

	// 2. Handle captures + check promotions + check en passant
	constexpr std::array<std::pair<int, int>, 2> captureDetails =
		{ std::pair{capture1Delta, capture1WrongFile}, std::pair{capture2Delta, capture2WrongFile} };

	for (const auto& [delta, wrongFile] : captureDetails) {
		target = home + delta;

		if ((file != wrongFile) && ((ColorOfPiece(GetPieceAt(target)) == opponentPieceColor) || (target == EnPassantSquare))) {
			if (GetSquareRank(target) != promotionRank) {
				const uint8_t moveFlag = (target == EnPassantSquare) ? MoveFlag::EnPassantPerformed : MoveFlag::None;
				moves.pushUnscored(Move(home, target, moveFlag));
			}
			else { // Promote
				moves.pushUnscored(Move(home, target, MoveFlag::PromotionToQueen));
				if constexpr (moveGen == MoveGen::All) {
					moves.pushUnscored(Move(home, target, MoveFlag::PromotionToRook));
					moves.pushUnscored(Move(home, target, MoveFlag::PromotionToBishop));
					moves.pushUnscored(Move(home, target, MoveFlag::PromotionToKnight));
				}
			}
		}
	}

	// 3. Handle double pushes
	if (rank == doublePushRank) {
		const bool free1 = GetPieceAt(home + forwardDelta) == Piece::None;
		const bool free2 = GetPieceAt(home + doublePushDelta) == Piece::None;
		if (free1 && free2) {
			if constexpr (moveGen == MoveGen::All) moves.pushUnscored(Move(home, home + doublePushDelta, MoveFlag::EnPassantPossible));
		}
	}

}

template <bool side>
void Board::GenerateCastlingMoves(MoveList& moves) const {

	using namespace Squares;

	constexpr auto shortEmpty = (side == Side::White) ? std::array{ F1, G1 } : std::array{ F8, G8 };
	constexpr auto shortSafe = (side == Side::White) ? std::array{ E1, F1, G1 } : std::array{ E8, F8, G8 };
	constexpr auto longEmpty = (side == Side::White) ? std::array{ B1, C1, D1 } : std::array{ B8, C8, D8 };
	constexpr auto longSafe = (side == Side::White) ? std::array{ C1, D1, E1 } : std::array{ C8, D8, E8 };
	constexpr int from = (side == Side::White) ? E1 : E8;
	constexpr int shortTo = (side == Side::White) ? G1 : G8;
	constexpr int longTo = (side == Side::White) ? C1 : C8;

	const bool rightToShortCastle = (side == Side::White) ? WhiteRightToShortCastle : BlackRightToShortCastle;
	const bool rightToLongCastle = (side == Side::White) ? WhiteRightToLongCastle : BlackRightToLongCastle;

	if (rightToShortCastle) {
		const bool empty = std::all_of(shortEmpty.begin(), shortEmpty.end(), [&](const int sq) {
			return GetPieceAt(sq) == Piece::None;
		});
		if (empty) {
			const bool safe = std::all_of(shortSafe.begin(), shortSafe.end(), [&](const int sq) {
				return !IsSquareAttacked<!side>(sq);
			});
			if (safe) moves.pushUnscored(Move(from, shortTo, MoveFlag::ShortCastle));
		}
	}
	if (rightToLongCastle) {
		const bool empty = std::all_of(longEmpty.begin(), longEmpty.end(), [&](const int sq) {
			return GetPieceAt(sq) == Piece::None;
		});
		if (empty) {
			const bool safe = std::all_of(longSafe.begin(), longSafe.end(), [&](const int sq) {
				return !IsSquareAttacked<!side>(sq);
			});
			if (safe) moves.pushUnscored(Move(from, longTo, MoveFlag::LongCastle));
		}
	}
}

void Board::GenerateMoves(MoveList& moves, const MoveGen moveGen, const Legality legality) {

	if (legality == Legality::Pseudolegal) {
		if (moveGen == MoveGen::All) {
			if (Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::All>(moves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::All>(moves);
		}
		else if (moveGen == MoveGen::Noisy) {
			if (Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::Noisy>(moves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::Noisy>(moves);
		}
	}
	else {
		MoveList legalMoves{};
		if (moveGen == MoveGen::All) {
			if (Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::All>(legalMoves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::All>(legalMoves);
		}
		else if (moveGen == MoveGen::Noisy) {
			if (Turn == Side::White) GeneratePseudolegalMoves<Side::White, MoveGen::Noisy>(legalMoves);
			else GeneratePseudolegalMoves<Side::Black, MoveGen::Noisy>(legalMoves);
		}
		for (const auto& m : legalMoves) {
			if (IsLegalMove(m.move)) moves.pushUnscored(m.move);
		}
	}
}

template <bool side, MoveGen moveGen>
void Board::GeneratePseudolegalMoves(MoveList& moves) const {
	const uint64_t whiteOccupancy = GetOccupancy(Side::White);
	const uint64_t blackOccupancy = GetOccupancy(Side::Black);
	uint64_t friendlyOccupancy = (Turn == Side::White) ? whiteOccupancy : blackOccupancy;

	while (friendlyOccupancy != 0) {
		const int sq = Popsquare(friendlyOccupancy);
		const int type = TypeOfPiece(GetPieceAt(sq));

		switch (type) {
		case PieceType::Pawn:
			GeneratePawnMoves<side, moveGen>(moves, sq);
			break;
		case PieceType::Knight:
			GenerateKnightMoves<side, moveGen>(moves, sq);
			break;
		case PieceType::Bishop:
			GenerateSlidingMoves<side, PieceType::Bishop, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
			break;
		case PieceType::Rook:
			GenerateSlidingMoves<side, PieceType::Rook, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
			break;
		case PieceType::Queen:
			GenerateSlidingMoves<side, PieceType::Queen, moveGen>(moves, sq, whiteOccupancy, blackOccupancy);
			break;
		case PieceType::King:
			GenerateKingMoves<side, moveGen>(moves, sq);
			if constexpr (moveGen == MoveGen::All) GenerateCastlingMoves<side>(moves);
			break;

		}
	}
}

// Generating attack maps -------------------------------------------------------------------------

template <bool attackingSide>
uint64_t Board::CalculateAttackedSquaresTemplated() const {

	uint64_t map = 0;

	// Pawn attacks
	map = (attackingSide == Side::White) ? GetPawnAttacks<Side::White>() : GetPawnAttacks<Side::Black>();

	// Knight attacks
	uint64_t knightBits = (attackingSide == Side::White) ? WhiteKnightBits : BlackKnightBits;
	while (knightBits) {
		const uint8_t sq = Popsquare(knightBits);
		map |= GenerateKnightAttacks(sq);
	}

	// King attacks
	uint8_t kingSquare = 63 - Lzcount((attackingSide == Side::White) ? WhiteKingBits : BlackKingBits);
	map |= KingMoveBits[kingSquare];

	// Sliding pieces
	// 'parallel' comes from being parallel to the axes, better name suggestions welcome
	uint64_t occ = GetOccupancy();
	uint64_t parallelSliders = (attackingSide == Side::White) ? (WhiteRookBits | WhiteQueenBits) : (BlackRookBits | BlackQueenBits);
	uint64_t diagonalSliders = (attackingSide == Side::White) ? (WhiteBishopBits | WhiteQueenBits) : (BlackBishopBits | BlackQueenBits);

	while (parallelSliders) {
		const uint8_t sq = Popsquare(parallelSliders);
		map |= GetRookAttacks(sq, occ);
	}
	while (diagonalSliders) {
		const uint8_t sq = Popsquare(diagonalSliders);
		map |= GetBishopAttacks(sq, occ);
	}

	return map; 
}

uint64_t Board::CalculateAttackedSquares(const bool attackingSide) const {
	return (attackingSide == Side::White) ? CalculateAttackedSquaresTemplated<Side::White>() : CalculateAttackedSquaresTemplated<Side::Black>();
}

uint64_t Board::GenerateKnightAttacks(const int from) const {
	return KnightMoveBits[from];
}

uint64_t Board::GenerateKingAttacks(const int from) const {
	return KingMoveBits[from];
}

template <bool attackingSide>
bool Board::IsSquareAttacked(const uint8_t square) const {
	uint64_t occupancy = GetOccupancy();

	if constexpr (attackingSide == Side::White) {
		// Attacked by a knight?
		if (KnightMoveBits[square] & WhiteKnightBits) return true;
		// Attacked by a king?
		if (KingMoveBits[square] & WhiteKingBits) return true;
		// Attacked by a pawn?
		if (SquareBit(square) & ((WhitePawnBits & ~File[0]) << 7)) return true;
		if (SquareBit(square) & ((WhitePawnBits & ~File[7]) << 9)) return true;
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
		if (SquareBit(square) & ((BlackPawnBits & ~File[0]) >> 9)) return true;
		if (SquareBit(square) & ((BlackPawnBits & ~File[7]) >> 7)) return true;
		// Attacked by a sliding piece?
		if (GetRookAttacks(square, occupancy) & (BlackRookBits | BlackQueenBits)) return true;
		if (GetBishopAttacks(square, occupancy) & (BlackBishopBits | BlackQueenBits)) return true;
		// Okay
		return false;
	}
}

uint64_t Board::GetAttackersOfSquare(const uint8_t square, const uint64_t occupied) const {
	// if not being used for SEE: occupied = GetOccupancy();

	// Generate attackers setwise
	// Taking advantage of the fact that for non-pawns, if X attacks Y, then Y attacks X
	const uint64_t pawnAttackers = (WhitePawnAttacks[square] & BlackPawnBits) | (BlackPawnAttacks[square] & WhitePawnBits);
	const uint64_t knightAttackers = KnightMoveBits[square] & (WhiteKnightBits | BlackKnightBits);
	const uint64_t bishopAttackers = GetBishopAttacks(square, occupied) & (WhiteBishopBits | BlackBishopBits | WhiteQueenBits | BlackQueenBits);
	const uint64_t rookAttackers = GetRookAttacks(square, occupied) & (WhiteRookBits | BlackRookBits | WhiteQueenBits | BlackQueenBits);
	const uint64_t kingAttackers = KingMoveBits[square] & (WhiteKingBits | BlackKingBits);
	return pawnAttackers | knightAttackers | bishopAttackers | rookAttackers | kingAttackers;
}

// Other ------------------------------------------------------------------------------------------

bool Board::IsDraw(const bool threefold) const {
	// 1. Fifty moves without progress
	if (HalfmoveClock >= 100) return true;

	// 2. Threefold repetitions
	const int64_t stateCount = std::count(PreviousHashes.begin(), PreviousHashes.end(), HashValue);
	if (stateCount >= (threefold ? 3 : 2)) return true;

	// 3. Insufficient material check
	// - has pawns or major pieces -> sufficient
	if (WhitePawnBits | BlackPawnBits) return false;
	if (WhiteRookBits | BlackRookBits | WhiteQueenBits | BlackQueenBits) return false;
	// - less than 4 with minor pieces is a draw, more than 4 is not
	const int pieceCount = Popcount(GetOccupancy());
	if (pieceCount > 4) return false;
	if (pieceCount < 4) return true;
	// - for exactly 4 pieces, check for same-color KBvKB
	if (Popcount(WhiteBishopBits & LightSquares) == 1 && Popcount(BlackBishopBits & LightSquares) == 1) return true;
	if (Popcount(WhiteBishopBits & DarkSquares) == 1 && Popcount(BlackBishopBits & DarkSquares) == 1) return true;
	return false;
}

GameState Board::GetGameState() {

	// Check checkmates & stalemates
	MoveList moves{};
	GenerateMoves(moves, MoveGen::All, Legality::Legal);
	if (moves.size() == 0) {
		if (IsInCheck()) {
			if (Turn == Side::Black) return GameState::WhiteVictory;
			else return GameState::BlackVictory;
		}
		else {
			return GameState::Draw; // Stalemate
		}
	}

	// Check other types of draws
	if (IsDraw(true)) return GameState::Draw;
	else return GameState::Playing;
}

std::string Board::GetFEN() const {
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

	result += (Turn == Side::White) ? " w " : " b ";

	bool castlingPossible = false;
	if (WhiteRightToShortCastle) { result += 'K'; castlingPossible = true; }
	if (WhiteRightToLongCastle) { result += 'Q'; castlingPossible = true; }
	if (BlackRightToShortCastle) { result += 'k'; castlingPossible = true; }
	if (BlackRightToLongCastle) { result += 'q'; castlingPossible = true; }
	if (!castlingPossible) result += '-';
	result += ' ';

	bool enPassantPossible = false;
	if ((EnPassantSquare != -1) && (Turn == Side::White)) {
		const bool fromRight = (((WhitePawnBits & ~File[0]) << 7) & SquareBit(EnPassantSquare));
		const bool fromLeft = (((WhitePawnBits & ~File[7]) << 9) & SquareBit(EnPassantSquare));
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if ((EnPassantSquare != -1) && (Turn == Side::Black)) {
		const bool fromRight = (((BlackPawnBits & ~File[0]) >> 9) & SquareBit(EnPassantSquare));
		const bool fromLeft = (((BlackPawnBits & ~File[7]) >> 7) & SquareBit(EnPassantSquare));
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if (enPassantPossible) result += SquareStrings[EnPassantSquare];
	else result += '-';

	result += ' ' + std::to_string(HalfmoveClock) + ' ' + std::to_string(FullmoveClock);
	return result;
}

int Board::GetPlys() const {
	return (FullmoveClock - 1) * 2 + (Turn == Side::White ? 0 : 1);
}

template <bool side>
uint8_t Board::GetKingSquare() const {
	if constexpr (side == Side::White) return LsbSquare(WhiteKingBits);
	else return LsbSquare(BlackKingBits);
}

bool Board::IsInCheck() const {
	if (Turn == Side::White) return IsSquareAttacked<Side::Black>(LsbSquare(WhiteKingBits));
	else return IsSquareAttacked<Side::White>(LsbSquare(BlackKingBits));
}