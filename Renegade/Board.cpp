#include "Board.h"

#pragma once

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
	PastHashes = std::vector<uint64_t>();

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
	AttackedSquares = CalculateAttackedSquares(TurnToPieceColor(!Turn));

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
	PastHashes.push_back(HashValue);
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

	AttackedSquares = b.AttackedSquares;
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
	PastHashes.reserve(b.PastHashes.size() + 1);
	PastHashes = b.PastHashes;
}

Board Board::Copy() {
	return Board(*this);
}


const void Board::Draw(const uint64_t customBits = 0) {

	const std::string side = Turn ? "white" : "black";
	cout << "    Move: " << FullmoveClock << " - " << side << " to play" << endl;;
	
	const std::string WhiteOnLightSquare = "\033[31;47m";
	const std::string WhiteOnDarkSquare = "\033[31;43m";
	const std::string BlackOnLightSquare = "\033[30;47m";
	const std::string BlackOnDarkSquare = "\033[30;43m";
	const std::string Default = "\033[0m";
	const std::string WhiteOnTarget = "\033[31;45m";
	const std::string BlackOnTarget = "\033[30;45m";

	cout << "    ------------------------ " << endl;
	// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
	for (int i = 7; i >= 0; i--) {
		cout << " " << i+1 << " |";
		for (int j = 0; j <= 7; j++) {
			const int pieceId = GetPieceAt(i * 8 + j);
			const int pieceColor = ColorOfPiece(pieceId);
			char piece = PieceChars[pieceId];

			std::string CellStyle;

			if ((i + j) % 2 == 1) {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnLightSquare;
				else CellStyle = WhiteOnLightSquare;
			}
			else {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnDarkSquare;
				else CellStyle = WhiteOnDarkSquare;
			}

			if (CheckBit(customBits, static_cast<uint64_t>(i) * 8 + static_cast<uint64_t>(j) )) {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnTarget;
				else  CellStyle = WhiteOnTarget;
			}

			cout << CellStyle << ' ' << piece << ' ' << Default;

		}
		cout << "|" << endl;
	}
	cout << "    ------------------------ " << endl;
	cout << "     a  b  c  d  e  f  g  h" << endl;

}

const std::string Board::GetFEN() {
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
		bool fromRight = (((WhitePawnBits & ~Bitboards::FileA) << 7) & SquareBits[EnPassantSquare]) != 0ULL;
		bool fromLeft = (((WhitePawnBits & ~Bitboards::FileH) << 9) & SquareBits[EnPassantSquare]) != 0ULL;
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if ((EnPassantSquare != -1) && (Turn == Turn::Black)) {
		bool fromRight = (((BlackPawnBits & ~Bitboards::FileA) >> 9) & SquareBits[EnPassantSquare]) != 0ULL;
		bool fromLeft = (((BlackPawnBits & ~Bitboards::FileH) >> 7) & SquareBits[EnPassantSquare]) != 0ULL;
		if (fromLeft || fromRight) enPassantPossible = true;
	}
	if (enPassantPossible) result += SquareStrings[EnPassantSquare];
	else result += '-';

	result += ' ' + std::to_string(HalfmoveClock) + ' ' + std::to_string(FullmoveClock);
	return result;
}

const uint64_t Board::HashInternal() {
	uint64_t hash = 0;

	// Pieces
	uint64_t occupancy = GetOccupancy();
	while (Popcount(occupancy) != 0) {
		uint64_t sq = 63ULL - Lzcount(occupancy);
		SetBitFalse(occupancy, sq);

		if (CheckBit(BlackPawnBits, sq)) hash ^= Zobrist[64 * 0 + sq];
		else if (CheckBit(WhitePawnBits, sq)) hash ^= Zobrist[64 * 1 + sq];
		else if (CheckBit(BlackKnightBits, sq)) hash ^= Zobrist[64 * 2 + sq];
		else if (CheckBit(WhiteKnightBits, sq)) hash ^= Zobrist[64 * 3 + sq];
		else if (CheckBit(BlackBishopBits, sq)) hash ^= Zobrist[64 * 4 + sq];
		else if (CheckBit(WhiteBishopBits, sq)) hash ^= Zobrist[64 * 5 + sq];
		else if (CheckBit(BlackRookBits, sq)) hash ^= Zobrist[64 * 6 + sq];
		else if (CheckBit(WhiteRookBits, sq)) hash ^= Zobrist[64 * 7 + sq];
		else if (CheckBit(BlackQueenBits, sq)) hash ^= Zobrist[64 * 8 + sq];
		else if (CheckBit(WhiteQueenBits, sq)) hash ^= Zobrist[64 * 9 + sq];
		else if (CheckBit(BlackKingBits, sq)) hash ^= Zobrist[64 * 10 + sq];
		else if (CheckBit(WhiteKingBits, sq)) hash ^= Zobrist[64 * 11 + sq];
	}

	// Castling
	if (WhiteRightToShortCastle) hash ^= Zobrist[768];
	if (WhiteRightToLongCastle) hash ^= Zobrist[769];
	if (BlackRightToShortCastle) hash ^= Zobrist[770];
	if (BlackRightToLongCastle) hash ^= Zobrist[771];

	// En passant
	if (EnPassantSquare != -1) {
		bool hasPawn = false;
		if (GetSquareFile(EnPassantSquare) != 0) {
			if (Turn == Turn::White) hasPawn = CheckBit(WhitePawnBits, EnPassantSquare - 7ULL);
			else hasPawn = CheckBit(BlackPawnBits, EnPassantSquare + 9ULL);
		}
		if ((GetSquareFile(EnPassantSquare) != 7) && !hasPawn) {
			if (Turn == Turn::White) hasPawn = CheckBit(WhitePawnBits, EnPassantSquare - 9ULL);
			else hasPawn = CheckBit(BlackPawnBits, EnPassantSquare + 7ULL);
		}
		if (hasPawn) hash ^= Zobrist[772 + GetSquareFile(EnPassantSquare)];
	}

	// Turn
	if (Turn == Turn::White) hash ^= Zobrist[780];

	return hash;
}

const uint64_t Board::Hash(const bool hashPlys) {
	if (hashPlys) return HashValue ^ (FullmoveClock * 0xE0C754AULL);
	return HashValue;
}

const int Board::GetPieceAt(const int place) {
	return OccupancyInts[place];
}

const int Board::GetPieceAtFromBitboards(const int place) {
	if (CheckBit(WhitePawnBits, place)) return Piece::WhitePawn;
	if (CheckBit(BlackPawnBits, place)) return Piece::BlackPawn;
	if (CheckBit(WhiteKnightBits, place)) return Piece::WhiteKnight;
	if (CheckBit(WhiteBishopBits, place)) return Piece::WhiteBishop;
	if (CheckBit(WhiteRookBits, place)) return Piece::WhiteRook;
	if (CheckBit(BlackKnightBits, place)) return Piece::BlackKnight;
	if (CheckBit(BlackBishopBits, place)) return Piece::BlackBishop;
	if (CheckBit(BlackRookBits, place)) return Piece::BlackRook;
	if (CheckBit(WhiteQueenBits, place)) return Piece::WhiteQueen;
	if (CheckBit(BlackQueenBits, place)) return Piece::BlackQueen;
	if (CheckBit(WhiteKingBits, place)) return Piece::WhiteKing;
	if (CheckBit(BlackKingBits, place)) return Piece::BlackKing;
	return 0;
}

void Board::GenerateOccupancy() {
	//for (int i = 0; i < 64; i++) OccupancyInts[i] = GetPieceAtFromBitboards(i);
	
	for (int i = 0; i < 64; i++) OccupancyInts[i] = 0;
	uint64_t bits = WhitePawnBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::WhitePawn;
		SetBitFalse(bits, sq);
	}
	bits = BlackPawnBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::BlackPawn;
		SetBitFalse(bits, sq);
	}
	bits = WhiteKnightBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::WhiteKnight;
		SetBitFalse(bits, sq);
	}
	bits = BlackKnightBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::BlackKnight;
		SetBitFalse(bits, sq);
	}
	bits = WhiteBishopBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::WhiteBishop;
		SetBitFalse(bits, sq);
	}
	bits = BlackBishopBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::BlackBishop;
		SetBitFalse(bits, sq);
	}
	bits = WhiteRookBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::WhiteRook;
		SetBitFalse(bits, sq);
	}
	bits = BlackRookBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::BlackRook;
		SetBitFalse(bits, sq);
	}
	bits = WhiteQueenBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::WhiteQueen;
		SetBitFalse(bits, sq);
	}
	bits = BlackQueenBits;
	while (Popcount(bits) != 0) {
		uint64_t sq = 63ULL - Lzcount(bits);
		OccupancyInts[sq] = Piece::BlackQueen;
		SetBitFalse(bits, sq);
	}

	uint64_t sq = 63ULL - Lzcount(WhiteKingBits);
	OccupancyInts[sq] = Piece::WhiteKing;

	sq = 63ULL - Lzcount(BlackKingBits);
	OccupancyInts[sq] = Piece::BlackKing;
}

const uint64_t Board::GetOccupancy() {
	return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits
		| BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

const uint64_t Board::GetOccupancy(const int pieceColor) {
	if (pieceColor == PieceColor::White) return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
	return BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

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

	std::vector<Move> legalMoves;
	legalMoves.reserve(7);
	GenerateLegalMoves(legalMoves, Turn);
	bool valid = false;
	for (const Move &m : legalMoves) {
		if ((m.to == move.to) && (m.from == move.from) && (m.flag == move.flag)) {
			valid = true;
			break;
		}
	}

	if (valid) {
		Push(move);
		return true;
	} else {
		return false;
	}

}

// Updating bitboard fields
void Board::TryMove(const Move move) {
	
	const int piece = GetPieceAt(move.from);
	const int pieceColor = ColorOfPiece(piece);
	const int pieceType = TypeOfPiece(piece);
	const int targetPiece = GetPieceAt(move.to);
	const int targetPieceColor = ColorOfPiece(targetPiece);
	const int targetPieceType = TypeOfPiece(targetPiece);

	if (piece == Piece::WhitePawn) { SetBitFalse(WhitePawnBits, move.from); SetBitTrue(WhitePawnBits, move.to); }
	else if (piece == Piece::WhiteKnight) { SetBitFalse(WhiteKnightBits, move.from); SetBitTrue(WhiteKnightBits, move.to); }
	else if (piece == Piece::WhiteBishop) { SetBitFalse(WhiteBishopBits, move.from); SetBitTrue(WhiteBishopBits, move.to); }
	else if (piece == Piece::WhiteRook) { SetBitFalse(WhiteRookBits, move.from); SetBitTrue(WhiteRookBits, move.to); }
	else if (piece == Piece::WhiteQueen) { SetBitFalse(WhiteQueenBits, move.from); SetBitTrue(WhiteQueenBits, move.to); }
	else if (piece == Piece::WhiteKing) { SetBitFalse(WhiteKingBits, move.from); SetBitTrue(WhiteKingBits, move.to); }
	else if (piece == Piece::BlackPawn) { SetBitFalse(BlackPawnBits, move.from); SetBitTrue(BlackPawnBits, move.to); }
	else if (piece == Piece::BlackKnight) { SetBitFalse(BlackKnightBits, move.from); SetBitTrue(BlackKnightBits, move.to); }
	else if (piece == Piece::BlackBishop) { SetBitFalse(BlackBishopBits, move.from); SetBitTrue(BlackBishopBits, move.to); }
	else if (piece == Piece::BlackRook) { SetBitFalse(BlackRookBits, move.from); SetBitTrue(BlackRookBits, move.to); }
	else if (piece == Piece::BlackQueen) { SetBitFalse(BlackQueenBits, move.from); SetBitTrue(BlackQueenBits, move.to); }
	else if (piece == Piece::BlackKing) { SetBitFalse(BlackKingBits, move.from); SetBitTrue(BlackKingBits, move.to); }

	if (targetPiece == Piece::WhitePawn) SetBitFalse(WhitePawnBits, move.to);
	else if (targetPiece == Piece::WhiteKnight) SetBitFalse(WhiteKnightBits, move.to);
	else if (targetPiece == Piece::WhiteBishop) SetBitFalse(WhiteBishopBits, move.to);
	else if (targetPiece == Piece::WhiteRook) SetBitFalse(WhiteRookBits, move.to);
	else if (targetPiece == Piece::WhiteQueen) SetBitFalse(WhiteQueenBits, move.to);
	else if (targetPiece == Piece::WhiteKing) SetBitFalse(WhiteKingBits, move.to); // ????
	else if (targetPiece == Piece::BlackPawn) SetBitFalse(BlackPawnBits, move.to);
	else if (targetPiece == Piece::BlackKnight) SetBitFalse(BlackKnightBits, move.to);
	else if (targetPiece == Piece::BlackBishop) SetBitFalse(BlackBishopBits, move.to);
	else if (targetPiece == Piece::BlackRook) SetBitFalse(BlackRookBits, move.to);
	else if (targetPiece == Piece::BlackQueen) SetBitFalse(BlackQueenBits, move.to);
	else if (targetPiece == Piece::BlackKing) SetBitFalse(BlackKingBits, move.to); // ????

	if ((piece == Piece::WhitePawn) && (move.to == EnPassantSquare) ) {
		SetBitFalse(BlackPawnBits, static_cast<uint64_t>((EnPassantSquare)-8ULL));
	}
	if ((piece == Piece::BlackPawn) && (move.to == EnPassantSquare)) {
		SetBitFalse(WhitePawnBits, static_cast<uint64_t>((EnPassantSquare)+8ULL));
	}

	if (piece == Piece::WhitePawn) {
		if (move.flag == MoveFlag::PromotionToKnight) { SetBitFalse(WhitePawnBits, move.to);  SetBitTrue(WhiteKnightBits, move.to); }
		else if (move.flag == MoveFlag::PromotionToBishop) { SetBitFalse(WhitePawnBits, move.to);  SetBitTrue(WhiteBishopBits, move.to); }
		else if (move.flag == MoveFlag::PromotionToRook) { SetBitFalse(WhitePawnBits, move.to);  SetBitTrue(WhiteRookBits, move.to); }
		else if (move.flag == MoveFlag::PromotionToQueen) { SetBitFalse(WhitePawnBits, move.to);  SetBitTrue(WhiteQueenBits, move.to); }
	}
	if (piece == Piece::BlackPawn) {
		if (move.flag == MoveFlag::PromotionToKnight) { SetBitFalse(BlackPawnBits, move.to);  SetBitTrue(BlackKnightBits, move.to); }
		else if (move.flag == MoveFlag::PromotionToBishop) { SetBitFalse(BlackPawnBits, move.to);  SetBitTrue(BlackBishopBits, move.to); }
		else if (move.flag == MoveFlag::PromotionToRook) { SetBitFalse(BlackPawnBits, move.to);  SetBitTrue(BlackRookBits, move.to); }
		else if (move.flag == MoveFlag::PromotionToQueen) { SetBitFalse(BlackPawnBits, move.to);  SetBitTrue(BlackQueenBits, move.to); }
	}

	if (targetPiece != 0) HalfmoveClock = 0;
	if (TypeOfPiece(piece) == PieceType::Pawn) HalfmoveClock = 0;
	if (move.flag == MoveFlag::EnPassantPerformed) HalfmoveClock = 0;

	// 2. Handle castling
	if ((Turn == Turn::White) && (move.flag == MoveFlag::ShortCastle)) {
		SetBitFalse(WhiteRookBits, 7);
		SetBitTrue(WhiteRookBits, 5);
		WhiteRightToShortCastle = false;
		WhiteRightToLongCastle = false;
	}
	if ((Turn == Turn::White) && (move.flag == MoveFlag::LongCastle)) {
		SetBitFalse(WhiteRookBits, 0);
		SetBitTrue(WhiteRookBits, 3);
		WhiteRightToShortCastle = false;
		WhiteRightToLongCastle = false;
	}
	if ((Turn == Turn::Black) && (move.flag == MoveFlag::ShortCastle)) {
		SetBitFalse(BlackRookBits, 63);
		SetBitTrue(BlackRookBits, 61);
		BlackRightToShortCastle = false;
		BlackRightToLongCastle = false;
	}
	if ((Turn == Turn::Black) && (move.flag == MoveFlag::LongCastle)) {
		SetBitFalse(BlackRookBits, 56);
		SetBitTrue(BlackRookBits, 59);
		BlackRightToShortCastle = false;
		BlackRightToLongCastle = false;
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
	if ((move.to == 7) || (move.from == 7)) WhiteRightToShortCastle = false;
	if ((move.to == 0) || (move.from == 0)) WhiteRightToLongCastle = false;
	if ((move.to == 63) || (move.from == 63)) BlackRightToShortCastle = false;
	if ((move.to == 56) || (move.from == 56)) BlackRightToLongCastle = false;

	// 4. Update en passant
	if (move.flag == MoveFlag::EnPassantPossible) {
		if (Turn == Turn::White) {
			EnPassantSquare = move.to - 8;
		}
		else {
			EnPassantSquare = move.to + 8;
		}
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
		const uint64_t previousAttackMap = AttackedSquares;
		AttackedSquares = CalculateAttackedSquares(TurnToPieceColor(!Turn));
		//const bool hasMoves = AreThereLegalMoves(Turn, previousAttackMap);
		HashValue =	HashInternal();
		return;
	}
	GenerateOccupancy();

	// Increment timers
	Turn = !Turn;
	if (Turn == Turn::White) FullmoveClock += 1;

	// Get state after
	bool inCheck = false;
	AttackedSquares = CalculateAttackedSquares(TurnToPieceColor(!Turn));
	if (Turn == Turn::White) inCheck = (AttackedSquares & WhiteKingBits) != 0;
	else inCheck = (AttackedSquares & BlackKingBits) != 0;

	// Update threefold repetition list
	if (HalfmoveClock == 0) PastHashes.clear();
	HashValue = HashInternal();
	PastHashes.push_back(HashValue);

}


void Board::GenerateKnightMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly) {
	const auto lookup = KnightMoves[home];
	for (const int l : lookup) {
		if (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(Turn)) continue;
		if (!quiescenceOnly || (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(!Turn))) moves.push_back(Move(home, l));
	}
}

void Board::GenerateSlidingMoves(std::vector<Move>& moves, const int piece, const int home, const bool quiescenceOnly) {

	const int myColor = ColorOfPiece(piece);
	const int opponentColor = (myColor == PieceColor::White) ? PieceColor::Black : PieceColor::White;
	const uint64_t friendlyOccupance = GetOccupancy(myColor);
	const uint64_t opponentOccupance = GetOccupancy(opponentColor);

	int rankDirection;
	int fileDirection;

	const int pieceType = TypeOfPiece(piece);
	const int minDir = ((pieceType == PieceType::Rook) || (pieceType == PieceType::Queen)) ? 1 : 5;
	const int maxDir = ((pieceType == PieceType::Bishop) || (pieceType == PieceType::Queen)) ? 8 : 4;

	for (int direction = minDir; direction <= maxDir; direction++) {
		switch (direction) {
		case 1: { rankDirection = +1; fileDirection = 0; break; }
		case 2: { rankDirection = -1; fileDirection = 0; break; }
		case 3: { rankDirection = 0; fileDirection = +1; break; }
		case 4: { rankDirection = 0; fileDirection = -1; break; }
		case 5: { rankDirection = +1; fileDirection = +1; break; }
		case 6: { rankDirection = +1; fileDirection = -1; break; }
		case 7: { rankDirection = -1; fileDirection = +1; break; }
		case 8: { rankDirection = -1; fileDirection = -1; break; }
		}

		int file = GetSquareFile(home);
		int rank = GetSquareRank(home);

		for (int i = 1; i < 8; i++) {
			file += fileDirection;
			rank += rankDirection;
			if ((file > 7) || (file < 0) || (rank > 7) || (rank < 0)) break;
			int thatSquare = Square(rank, file);
			if (CheckBit(friendlyOccupance, thatSquare)) break;
			if (CheckBit(opponentOccupance, thatSquare)) {
				moves.push_back(Move(home, thatSquare));
				break;
			}
			if (!quiescenceOnly) moves.push_back(Move(home, thatSquare));
		}
	}

}

// Sliding attack generation, the idea being that this function does multiple pieces at once.
// Also who doesn't like this level of bit fiddling?
const uint64_t Board::GenerateSlidingAttacksShiftDown(const int direction, const uint64_t boundMask, const uint64_t propagatingPieces, const uint64_t friendlyPieces, const uint64_t opponentPieces) {
	const uint64_t blockingFriends = friendlyPieces & ~propagatingPieces;
	uint64_t fill = ((propagatingPieces & boundMask) >> direction) & ~blockingFriends;
	uint64_t lastFill = fill;
	for (int i = 0; i < 7; i++) {
		fill = fill & boundMask;
		fill |= fill >> direction;
		fill = fill & ~blockingFriends & ~(opponentPieces >> direction);
		if (fill == lastFill) return fill | (((propagatingPieces & boundMask) >> direction) & ~blockingFriends);
		lastFill = fill;
	}
	fill |= ((propagatingPieces & boundMask) >> direction) & ~blockingFriends;
	return fill;
}

const uint64_t Board::GenerateSlidingAttacksShiftUp(const int direction, const uint64_t boundMask, const uint64_t propagatingPieces, const uint64_t friendlyPieces, const uint64_t opponentPieces) {
	const uint64_t blockingFriends = friendlyPieces & ~propagatingPieces;
	uint64_t fill = ((propagatingPieces & boundMask) << direction) & ~blockingFriends;
	uint64_t lastFill = fill;
	for (int i = 0; i < 7; i++) {
		fill = fill & boundMask;
		fill |= fill << direction;
		fill = fill & ~blockingFriends & ~(opponentPieces << direction);
		if (fill == lastFill) return fill | (((propagatingPieces & boundMask) << direction) & ~blockingFriends);
		lastFill = fill;
	}
	fill |= ((propagatingPieces & boundMask) << direction) & ~blockingFriends;
	return fill;
}

const uint64_t Board::GenerateKnightAttacks(const int from) {
	return KnightMoveBits[from];
}

const uint64_t Board::GenerateKingAttacks(const int from) {
	return KingMoveBits[from];
}

void Board::GenerateKingMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly) {
	const auto lookup = KingMoves[home];
	for (const int l : lookup) {
		if (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(Turn)) continue;
		if (!quiescenceOnly || (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(!Turn))) moves.push_back(Move(home, l));
	}

}

void Board::GeneratePawnMoves(std::vector<Move>& moves, const int home, const bool quiescenceOnly) {
	const int piece = GetPieceAt(home);
	const int color = ColorOfPiece(piece);
	const int file = GetSquareFile(home);
	const int rank = GetSquareRank(home);
	int target;

	if (color == PieceColor::White) {
		// 1. Can move forward? + check promotions
		target = home + 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 7) {
				if (!quiescenceOnly || (SquareRankArray[target] == 6)) moves.push_back(Move(home, target));
			} else { // Promote
				moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
				moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
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
				moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
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
					moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
					moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
					moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 1) {
			bool free1 = GetPieceAt(home + 8) == 0;
			bool free2 = GetPieceAt(home + 16) == 0;
			if (free1 && free2) {
				if (!quiescenceOnly) moves.push_back(Move(home, home+16, MoveFlag::EnPassantPossible));
			}
		}
	}

	if (color == PieceColor::Black) {
		// 1. Can move forward? + check promotions
		target = home - 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 0) {
				if (!quiescenceOnly || (SquareRankArray[target] == 1)) moves.push_back(Move(home, target));
			} else { // Promote
				moves.push_back(Move(home, target, MoveFlag::PromotionToQueen));
				moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
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
				moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
				moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
				moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
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
					moves.push_back(Move(home, target, MoveFlag::PromotionToRook));
					moves.push_back(Move(home, target, MoveFlag::PromotionToBishop));
					moves.push_back(Move(home, target, MoveFlag::PromotionToKnight));
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 6) {
			bool free1 = GetPieceAt(home - 8) == 0;
			bool free2 = GetPieceAt(home - 16) == 0;
			if (free1 && free2) {
				if (!quiescenceOnly) moves.push_back(Move(home, home - 16, MoveFlag::EnPassantPossible));
			}
		}
	}
}

void Board::GenerateCastlingMoves(std::vector<Move>& moves) {
	if ((Turn == Turn::White) && (WhiteRightToShortCastle)) {
		const bool empty_f1 = GetPieceAt(Squares::F1) == 0;
		const bool empty_g1 = GetPieceAt(Squares::G1) == 0;
		const bool safe_e1 = CheckBit(AttackedSquares, Squares::E1) == 0;
		const bool safe_f1 = CheckBit(AttackedSquares, Squares::F1) == 0;
		const bool safe_g1 = CheckBit(AttackedSquares, Squares::G1) == 0;
		if (empty_f1 && empty_g1 && safe_e1 && safe_f1 && safe_g1) {
			Move m = Move(Squares::E1, Squares::G1, MoveFlag::ShortCastle);
			moves.push_back(m);
		}
	}
	if ((Turn == Turn::White) && (WhiteRightToLongCastle)) {
		const bool empty_b1 = GetPieceAt(Squares::B1) == 0;
		const bool empty_c1 = GetPieceAt(Squares::C1) == 0;
		const bool empty_d1 = GetPieceAt(Squares::D1) == 0;
		const bool safe_c1 = CheckBit(AttackedSquares, Squares::C1) == 0;
		const bool safe_d1 = CheckBit(AttackedSquares, Squares::D1) == 0;
		const bool safe_e1 = CheckBit(AttackedSquares, Squares::E1) == 0;
		if (empty_b1 && empty_c1 && empty_d1 && safe_c1 && safe_d1 && safe_e1) {
			Move m = Move(Squares::E1, Squares::C1, MoveFlag::LongCastle);
			moves.push_back(m);
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToShortCastle)) {
		const bool empty_f8 = GetPieceAt(Squares::F8) == 0;
		const bool empty_g8 = GetPieceAt(Squares::G8) == 0;
		const bool safe_e8 = CheckBit(AttackedSquares, Squares::E8) == 0;
		const bool safe_f8 = CheckBit(AttackedSquares, Squares::F8) == 0;
		const bool safe_g8 = CheckBit(AttackedSquares, Squares::G8) == 0;
		if (empty_f8 && empty_g8 && safe_e8 && safe_f8 && safe_g8) {
			Move m = Move(60, 62, MoveFlag::ShortCastle);
			moves.push_back(m);
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToLongCastle)) {
		const bool empty_b8 = GetPieceAt(Squares::B8) == 0;
		const bool empty_c8 = GetPieceAt(Squares::C8) == 0;
		const bool empty_d8 = GetPieceAt(Squares::D8) == 0;
		const bool safe_c8 = CheckBit(AttackedSquares, Squares::C8) == 0;
		const bool safe_d8 = CheckBit(AttackedSquares, Squares::D8) == 0;
		const bool safe_e8 = CheckBit(AttackedSquares, Squares::E8) == 0;
		if (empty_b8 && empty_c8 && empty_d8 && safe_c8 && safe_d8 && safe_e8) {
			Move m = Move(Squares::E8, Squares::C8, MoveFlag::LongCastle);
			moves.push_back(m);
		}
	}
}

// Attack maps are an integral part of this engine, they are used to check the legality of pseudolegal moves
const uint64_t Board::CalculateAttackedSquares(const int colorOfPieces) {
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
	squares |= GenerateSlidingAttacksShiftDown(1, ~Bitboards::FileA, parallelSliders, friendlyPieces, opponentPieces); // Left
	squares |= GenerateSlidingAttacksShiftDown(8, ~Bitboards::Rank1, parallelSliders, friendlyPieces, opponentPieces); // Down
	squares |= GenerateSlidingAttacksShiftUp(1, ~Bitboards::FileH, parallelSliders, friendlyPieces, opponentPieces); // Right
	squares |= GenerateSlidingAttacksShiftUp(8, ~Bitboards::Rank8, parallelSliders, friendlyPieces, opponentPieces); // Up
	squares |= GenerateSlidingAttacksShiftDown(7, ~Bitboards::FileH & ~Bitboards::Rank1, diagonalSliders, friendlyPieces, opponentPieces); // Right-down
	squares |= GenerateSlidingAttacksShiftDown(9, ~Bitboards::FileA & ~Bitboards::Rank1, diagonalSliders, friendlyPieces, opponentPieces); // Left-down
	squares |= GenerateSlidingAttacksShiftUp(9, ~Bitboards::FileH & ~Bitboards::Rank8, diagonalSliders, friendlyPieces, opponentPieces); // Right-up
	squares |= GenerateSlidingAttacksShiftUp(7, ~Bitboards::FileA & ~Bitboards::Rank8, diagonalSliders, friendlyPieces, opponentPieces); // Left-up

	// King attack gen
	uint64_t kingSquare = 63ULL - Lzcount(colorOfPieces == PieceColor::White ? WhiteKingBits : BlackKingBits);
	uint64_t fill = KingMoveBits[kingSquare];
	fill = fill & ~friendlyPieces;
	squares |= fill;

	// Knight attack gen: +6, +10, +15, +17
	uint64_t knightBits = colorOfPieces == PieceColor::White ? WhiteKnightBits : BlackKnightBits;
	fill = 0;
	while (Popcount(knightBits) != 0) {
		uint64_t sq = 63ULL - Lzcount(knightBits);
		fill |= GenerateKnightAttacks(static_cast<int>(sq));
		SetBitFalse(knightBits, sq);
	}
	fill = fill & ~friendlyPieces;
	squares |= fill;

	// Attack maps (by my own definition) doesn't cover friendly pieces
	squares = squares & ~friendlyPieces;

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

	return squares;

}

void Board::GenerateLegalMoves(std::vector<Move>& moves, const int turn) {
	std::vector<Move> legalMoves;

	GeneratePseudoLegalMoves(legalMoves, turn, false);
	for (const Move& m : legalMoves) {
		if (IsLegalMove(m, turn)) moves.push_back(m);
	}
}

void Board::GenerateNonQuietMoves(std::vector<Move>& moves, const int turn) {
	GeneratePseudoLegalMoves(moves, turn, true);
}

void Board::GeneratePseudoLegalMoves(std::vector<Move>& moves, const int turn, const bool quiescenceOnly) {
	const int myColor = TurnToPieceColor(turn);
	uint64_t occupancy = GetOccupancy(TurnToPieceColor(turn));
	while (occupancy != 0) {
		int i = 63 - Lzcount(occupancy);
		SetBitFalse(occupancy, i);

		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		if (type == PieceType::Pawn) GeneratePawnMoves(moves, i, quiescenceOnly);
		else if (type == PieceType::Knight) GenerateKnightMoves(moves, i, quiescenceOnly);
		else if (type == PieceType::Bishop) GenerateSlidingMoves(moves, piece, i, quiescenceOnly);
		else if (type == PieceType::Rook) GenerateSlidingMoves(moves, piece, i, quiescenceOnly);
		else if (type == PieceType::Queen) GenerateSlidingMoves(moves, piece, i, quiescenceOnly);
		else if (type == PieceType::King) {
			GenerateKingMoves(moves, i, quiescenceOnly);
			if (!quiescenceOnly) GenerateCastlingMoves(moves);
		}
	}
}

bool Board::AreThereLegalMoves(const bool turn) {
	bool hasMoves = false;
	const int myColor = TurnToPieceColor(turn);

	/*
	// Quick king test - if the king can move to a free square without being attacked, then we have a legal move
	const uint64_t kingBits = turn == Turn::White ? WhiteKingBits : BlackKingBits;
	const uint64_t sq = 63ULL - Lzcount(kingBits);
	const uint64_t kingMoveBits = GenerateKingAttacks(static_cast<int>(sq));
	const uint64_t fastKingCheck = (~previousAttackMap) & (~GetOccupancy()) & kingMoveBits;
	if (fastKingCheck != 0) return true;*/

	std::vector<Move> moves;
	uint64_t occupancy = GetOccupancy(TurnToPieceColor(turn));
	while (occupancy != 0) {
		int i = 63 - Lzcount(occupancy);
		SetBitFalse(occupancy, i);
		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		if (type == PieceType::Pawn) GeneratePawnMoves(moves, i, false);
		else if (type == PieceType::Knight) GenerateKnightMoves(moves, i, false);
		else if (type == PieceType::Bishop) GenerateSlidingMoves(moves, piece, i, false);
		else if (type == PieceType::Rook) GenerateSlidingMoves(moves, piece, i, false);
		else if (type == PieceType::Queen) GenerateSlidingMoves(moves, piece, i, false);
		else if (type == PieceType::King) {
			GenerateKingMoves(moves, i, false);
			GenerateCastlingMoves(moves);
		}

		if (moves.size() != 0) {
			for (Move m : moves) {
				if (IsLegalMove(m, turn)) {
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

// We try to call this function as little as possible
// Pretends to make a move, check its legality and then revert the variables
// It only cares about whether the king will be in check, completely invalid moves won't be noticed
bool Board::IsLegalMove(const Move m, const int turn) {
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
	const uint64_t attackedSquares = AttackedSquares;

	// Push move
	TryMove(m);

	// Check
	bool inCheck = false;
	if (Turn == Turn::White) {
		AttackedSquares = CalculateAttackedSquares(PieceColor::Black);
		inCheck = (AttackedSquares & WhiteKingBits) != 0;
	} else {
		AttackedSquares = CalculateAttackedSquares(PieceColor::White);
		inCheck = (AttackedSquares & BlackKingBits) != 0;
	}

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
	AttackedSquares = attackedSquares;

	return !inCheck;
}

const bool Board::IsMoveQuiet(const Move& move) {
	if (GetPieceAt(move.to) != Piece::None) return false;
	if ((move.flag == MoveFlag::PromotionToQueen) || (move.flag == MoveFlag::PromotionToKnight) || (move.flag == MoveFlag::PromotionToRook) || (move.flag == MoveFlag::PromotionToBishop)) return false;
	if ((GetPieceAt(move.from) == Piece::WhitePawn) && (SquareRankArray[move.to] >= 5)) return false;
	if ((GetPieceAt(move.from) == Piece::BlackPawn) && (SquareRankArray[move.to] <= 2)) return false;
	if (move.flag == MoveFlag::EnPassantPerformed) return false;
	return true;
}

const bool Board::IsDraw() {

	// Threefold repetitions
	const int64_t stateCount = std::count(PastHashes.end() - HalfmoveClock, PastHashes.end(), HashValue);
	if (stateCount >= 3) {
		return true;
	}

	// Insufficient material check
	// I think this neglects some cases when pawns can't move
	const int pieceCount = Popcount(GetOccupancy());
	if (pieceCount <= 4) {
		const int WhitePawnCount = Popcount(WhitePawnBits);
		const int WhiteKnightCount = Popcount(WhiteKnightBits);
		const int WhiteBishopCount = Popcount(WhiteBishopBits);
		const int WhiteRookCount = Popcount(WhiteRookBits);
		const int WhiteQueenCount = Popcount(WhiteQueenBits);
		const int BlackPawnCount = Popcount(BlackPawnBits);
		const int BlackKnightCount = Popcount(BlackKnightBits);
		const int BlackBishopCount = Popcount(BlackBishopBits);
		const int BlackRookCount = Popcount(BlackRookBits);
		const int BlackQueenCount = Popcount(BlackQueenBits);
		const int WhitePieceCount = WhitePawnCount + WhiteKnightCount + WhiteBishopCount + WhiteRookCount + WhiteQueenCount;
		const int BlackPieceCount = BlackPawnCount + BlackKnightCount + BlackBishopCount + BlackRookCount + BlackQueenCount;
		const int WhiteLightBishopCount = Popcount(WhiteBishopBits & Bitboards::LightSquares);
		const int WhiteDarkBishopCount = Popcount(WhiteBishopBits & Bitboards::DarkSquares);
		const int BlackLightBishopCount = Popcount(BlackBishopBits & Bitboards::LightSquares);
		const int BlackDarkBishopCount = Popcount(BlackBishopBits & Bitboards::DarkSquares);

		if (WhitePieceCount == 0 && BlackPieceCount == 0) return true;
		if (WhitePieceCount == 1 && WhiteKnightCount == 1 && BlackPieceCount == 0) return true;
		if (BlackPieceCount == 1 && BlackKnightCount == 1 && WhitePieceCount == 0) return true;
		if (WhitePieceCount == 1 && WhiteBishopCount == 1 && BlackPieceCount == 0) return true;
		if (BlackPieceCount == 1 && BlackBishopCount == 1 && WhitePieceCount == 0) return true;
		if (WhitePieceCount == 1 && WhiteLightBishopCount == 1 && BlackPieceCount == 1 && BlackLightBishopCount == 1) return true;
		if (WhitePieceCount == 1 && WhiteDarkBishopCount == 1 && BlackPieceCount == 1 && BlackDarkBishopCount == 1) return true;
	}

	// Fifty moves without progress
	if (HalfmoveClock >= 100) return true;

	// Return false otherwise
	return false;
}

const GameState Board::GetGameState() {

	// Check checkmates & stalemates
	if (!AreThereLegalMoves(Turn)) {
		bool inCheck = false;
		AttackedSquares = CalculateAttackedSquares(TurnToPieceColor(!Turn));
		if (Turn == Turn::White) inCheck = (AttackedSquares & WhiteKingBits) != 0;
		else inCheck = (AttackedSquares & BlackKingBits) != 0;

		if (inCheck) {
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