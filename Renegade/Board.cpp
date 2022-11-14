#include "Board.h"

#pragma once

Board::Board(string fen) {

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
	State = GameState::Playing;
	StartingPosition = (fen == starting_fen);

	std::stringstream ss(fen);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	std::vector<std::string> parts(begin, end);
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

	if (Turn == Turn::White) {
		AttackedSquares = CalculateAttackedSquares(PieceColor::Black);
	} else {
		AttackedSquares = CalculateAttackedSquares(PieceColor::White);
	}

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
}

Board::Board() {
	Board(starting_fen);
	StartingPosition = true;
}

Board::Board(const Board &b) {
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
	State = b.State;
	DrawCheck = b.DrawCheck;
	StartingPosition = b.StartingPosition;
}

void Board::Draw(unsigned __int64 customBits = 0) {

	string side = Turn ? "white" : "black";
	cout << "    Move: " << FullmoveClock << " - " << side << " to play" << endl;;
	
	const string WhiteOnLightSqaure = "\033[31;47m";
	const string WhiteOnDarkSqaure = "\033[31;43m";
	const string BlackOnLightSqaure = "\033[30;47m";
	const string BlackOnDarkSqaure = "\033[30;43m";
	const string Default = "\033[0m";
	const string WhiteOnTarget = "\033[31;45m";
	const string BlackOnTarget = "\033[30;45m";

	cout << "    ------------------------ " << endl;
	// https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
	for (int i = 7; i >= 0; i--) {
		cout << " " << i+1 << " |";
		for (int j = 0; j <= 7; j++) {
			int pieceId = GetPieceAt(i * 8 + j);
			int pieceColor = ColorOfPiece(pieceId);
			char piece = ' ';
			switch (pieceId) {
			case Piece::WhitePawn: piece = 'P'; break;
			case Piece::WhiteKnight: piece = 'N'; break;
			case Piece::WhiteBishop: piece = 'B'; break;
			case Piece::WhiteRook: piece = 'R'; break;
			case Piece::WhiteQueen: piece = 'Q'; break;
			case Piece::WhiteKing: piece = 'K'; break;
			case Piece::BlackPawn: piece = 'p'; break;
			case Piece::BlackKnight: piece = 'n'; break;
			case Piece::BlackBishop: piece = 'b'; break;
			case Piece::BlackRook: piece = 'r'; break;
			case Piece::BlackQueen: piece = 'q'; break;
			case Piece::BlackKing: piece = 'k'; break;
			}

			string CellStyle;

			if ((i + j) % 2 == 1) {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnLightSqaure;
				else CellStyle = WhiteOnLightSqaure;
			}
			else {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnDarkSqaure;
				else CellStyle = WhiteOnDarkSqaure;
			}

			if (CheckBit(customBits, (unsigned __int64)i * 8 + j)) {
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

unsigned __int64 Board::Hash(bool hashPlys) {
	unsigned __int64 hash = 0ULL;

	// Pieces
	for (int i = 0; i < 64; i++) {
		if (CheckBit(BlackPawnBits, i)) hash ^= Zobrist[64 * 0 + i];
		else if (CheckBit(WhitePawnBits, i)) hash ^= Zobrist[64 * 1 + i];
		else if (CheckBit(BlackKnightBits, i)) hash ^= Zobrist[64 * 2 + i];
		else if (CheckBit(WhiteKnightBits, i)) hash ^= Zobrist[64 * 3 + i];
		else if (CheckBit(BlackBishopBits, i)) hash ^= Zobrist[64 * 4 + i];
		else if (CheckBit(WhiteBishopBits, i)) hash ^= Zobrist[64 * 5 + i];
		else if (CheckBit(BlackRookBits, i)) hash ^= Zobrist[64 * 6 + i];
		else if (CheckBit(WhiteRookBits, i)) hash ^= Zobrist[64 * 7 + i];
		else if (CheckBit(BlackQueenBits, i)) hash ^= Zobrist[64 * 8 + i];
		else if (CheckBit(WhiteQueenBits, i)) hash ^= Zobrist[64 * 9 + i];
		else if (CheckBit(BlackKingBits, i)) hash ^= Zobrist[64 * 10 + i];
		else if (CheckBit(WhiteKingBits, i)) hash ^= Zobrist[64 * 11 + i];
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

	// Ply number - internal only
	if (hashPlys) hash ^= (FullmoveClock * 0xE0C754A);

	return hash;
}

int Board::GetPieceAt(int place) {
	if (CheckBit(WhitePawnBits, place)) return Piece::WhitePawn;
	if (CheckBit(WhiteKnightBits, place)) return Piece::WhiteKnight;
	if (CheckBit(WhiteBishopBits, place)) return Piece::WhiteBishop;
	if (CheckBit(WhiteRookBits, place)) return Piece::WhiteRook;
	if (CheckBit(WhiteQueenBits, place)) return Piece::WhiteQueen;
	if (CheckBit(WhiteKingBits, place)) return Piece::WhiteKing;
	if (CheckBit(BlackPawnBits, place)) return Piece::BlackPawn;
	if (CheckBit(BlackKnightBits, place)) return Piece::BlackKnight;
	if (CheckBit(BlackBishopBits, place)) return Piece::BlackBishop;
	if (CheckBit(BlackRookBits, place)) return Piece::BlackRook;
	if (CheckBit(BlackQueenBits, place)) return Piece::BlackQueen;
	if (CheckBit(BlackKingBits, place)) return Piece::BlackKing;
	return 0;
}

bool Board::PushUci(string ucistr) {

	int sq1 = SquareToNum(ucistr.substr(0, 2));
	int sq2 = SquareToNum(ucistr.substr(2, 2));
	int extra = ucistr[4];
	Move move = Move(sq1, sq2);
	int piece = GetPieceAt(sq1);

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
		if (abs(f2 - f1) > 1) move.flag = MoveFlag::EnPassant;
	}

	// En passant performed
	// ???

	std::vector<Move> legalMoves = GenerateLegalMoves(Turn);
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

void Board::TryMove(Move move) {
	
	int piece = GetPieceAt(move.from);
	int pieceColor = ColorOfPiece(piece);
	int pieceType = TypeOfPiece(piece);
	int targetPiece = GetPieceAt(move.to);
	int targetPieceColor = ColorOfPiece(targetPiece);
	int targetPieceType = TypeOfPiece(targetPiece);

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
		SetBitFalse(BlackPawnBits, (unsigned __int64)(EnPassantSquare)-8);
	}
	if ((piece == Piece::BlackPawn) && (move.to == EnPassantSquare)) {
		SetBitFalse(WhitePawnBits, (unsigned __int64)(EnPassantSquare)+8);
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
	if (move.flag == MoveFlag::EnPassant) {
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

void Board::Push(Move move) {

	TryMove(move);

	// Increment timers
	Turn = !Turn;
	HalfmoveClock += 1;
	if (Turn == Turn::White) FullmoveClock += 1;

	if (Turn == Turn::White) {
		AttackedSquares = CalculateAttackedSquares(PieceColor::Black);
	} else {
		AttackedSquares = CalculateAttackedSquares(PieceColor::White);
	}

	// Get state after
	int inCheck = 0;
	if (Turn == Turn::White) inCheck = NonZeros(AttackedSquares & WhiteKingBits);
	else inCheck = NonZeros(AttackedSquares & BlackKingBits);

	
	// Check checkmates & stalemates
	bool hasMoves = AreThereLegalMoves(Turn);
	if (!hasMoves) {
		if (inCheck) {
			if (Turn == Turn::Black) State = GameState::WhiteVictory;
			else State = GameState::BlackVictory;
		} else {
			State = GameState::Draw;
		}
	}
	if ((State != GameState::Playing) || (!DrawCheck)) return;

	// Insufficient material check
	int WhitePawnCount = NonZeros(WhitePawnBits);
	int WhiteKnightCount = NonZeros(WhiteKnightBits);
	int WhiteBishopCount = NonZeros(WhiteBishopBits);
	int WhiteRookCount = NonZeros(WhiteRookBits);
	int WhiteQueenCount = NonZeros(WhiteQueenBits);
	int BlackPawnCount = NonZeros(BlackPawnBits);
	int BlackKnightCount = NonZeros(BlackKnightBits);
	int BlackBishopCount = NonZeros(BlackBishopBits);
	int BlackRookCount = NonZeros(BlackRookBits);
	int BlackQueenCount = NonZeros(BlackQueenBits);
	int WhitePieceCount = WhitePawnCount + WhiteKnightCount + WhiteBishopCount + WhiteRookCount + WhiteQueenCount;
	int BlackPieceCount = BlackPawnCount + BlackKnightCount + BlackBishopCount + BlackRookCount + BlackQueenCount;
	int WhiteLightBishopCount = NonZeros(WhiteBishopBits & LightSquares);
	int WhiteDarkBishopCount = NonZeros(WhiteBishopBits & DarkSquares);
	int BlackLightBishopCount = NonZeros(BlackBishopBits & LightSquares);
	int BlackDarkBishopCount = NonZeros(BlackBishopBits & DarkSquares);

	bool flag = false;
	if (WhitePieceCount == 0 && BlackPieceCount == 0) flag = true;
	if (WhitePieceCount == 1 && WhiteKnightCount == 1 && BlackPieceCount == 0) flag = true;
	if (BlackPieceCount == 1 && BlackKnightCount == 1 && WhitePieceCount == 0) flag = true;
	if (WhitePieceCount == 1 && WhiteBishopCount == 1 && BlackPieceCount == 0) flag = true;
	if (BlackPieceCount == 1 && BlackBishopCount == 1 && WhitePieceCount == 0) flag = true;
	if (WhitePieceCount == 1 && WhiteLightBishopCount == 1 && BlackPieceCount == 1 && BlackLightBishopCount == 1) flag = true;
	if (WhitePieceCount == 1 && WhiteDarkBishopCount == 1 && BlackPieceCount == 1 && BlackDarkBishopCount == 1) flag = true;

	if (flag) State = GameState::Draw;
	if (State != GameState::Playing) return;
	
	// Check clocks...
	if (HalfmoveClock == 100) State = GameState::Draw;

}

unsigned unsigned __int64 Board::GenerateKnightAttacks(int from) {
	unsigned unsigned __int64 squares = 0ULL;
	return KnightMoveBits[from];
}

unsigned unsigned __int64 Board::GenerateKingAttacks(int from) {
	unsigned unsigned __int64 squares = 0ULL;
	return KingMoveBits[from];
}

unsigned unsigned __int64 Board::GenerateBishopAttacks(int pieceColor, int from) {
	unsigned unsigned __int64 squares = 0ULL;
	int rank = GetSquareRank(from);
	int file = GetSquareFile(from);
	// Go up-left
	int r = rank;
	int f = file;
	while ((r < 7) && (f > 0)) {
		r++;
		f--;
		int pos = Square(r, f);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	// Go up-right
	r = rank;
	f = file;
	while ((r < 7) && (f < 7)) {
		r++;
		f++;
		int pos = Square(r, f);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	// Go down-right
	r = rank;
	f = file;
	while ((r > 0) && (f < 7)) {
		r--;
		f++;
		int pos = Square(r, f);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	// Go down-left
	r = rank;
	f = file;
	while ((r > 0) && (f > 0)) {
		r--;
		f--;
		int pos = Square(r, f);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	return squares;
}

unsigned __int64 Board::GenerateRookAttacks(int pieceColor, int from) {
	unsigned __int64 squares = 0ULL;
	int rank = GetSquareRank(from);
	int file = GetSquareFile(from);
	
	// Go up
	for (int i = rank + 1; i < 8; i++) {
		int pos = Square(i, file);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	// Go down
	for (int i = rank - 1; i >= 0; i--) {
		int pos = Square(i, file);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	// Go left
	for (int i = file - 1; i >= 0; i--) {
		int pos = Square(rank, i);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}
	// Go right
	for (int i = file + 1; i < 8; i++) {
		int pos = Square(rank, i);
		int colorAtPos = ColorOfPiece(GetPieceAt(pos));
		if (colorAtPos == PieceColor::None) {
			SetBitTrue(squares, pos);
			continue;
		}
		if (pieceColor == colorAtPos) {
			break;
		}
		if (pieceColor != colorAtPos) {
			SetBitTrue(squares, pos);
			break;
		}
	}

	return squares;
}

unsigned __int64 Board::GenerateQueenAttacks(int pieceColor, int from) {
	return GenerateRookAttacks(pieceColor, from) | GenerateBishopAttacks(pieceColor, from);
}

unsigned __int64 Board::GeneratePawnAttacks(int pieceColor, int from) {
	unsigned __int64 squares = 0ULL;
	int rank = GetSquareRank(from);
	int file = GetSquareFile(from);
	if (pieceColor == PieceColor::White) {
		squares = WhitePawnAttacks[from];
		if ((from - 1 == EnPassantSquare) && (file != 0)) SetBitTrue(squares, (unsigned __int64)from - 1);
		if ((from + 1 == EnPassantSquare) && (file != 7)) SetBitTrue(squares, (unsigned __int64)from + 1);
	}
	else {
		squares = BlackPawnAttacks[from];
		if ((from - 1 == EnPassantSquare) && (file != 0)) SetBitTrue(squares, (unsigned __int64)from - 1);
		if ((from + 1 == EnPassantSquare) && (file != 7)) SetBitTrue(squares, (unsigned __int64)from + 1);
	}
	return squares;
}


std::vector<Move> Board::GenerateKnightMoves(int home) {
	auto lookup = KnightMoves[home];
	std::vector<Move> list;
	list.reserve(8);
	for (int l : lookup) {
		if (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(Turn)) continue;
		list.push_back(Move(home, l));
	}
	return list;
}

std::vector<Move> Board::GenerateSlidingMoves(int piece, int home) {
	unsigned __int64 friendlyOccupance = 0ULL;
	unsigned __int64 opponentOccupance = 0ULL;
	std::vector<Move> Moves;
	Moves.reserve(27);
	if (ColorOfPiece(piece) == PieceColor::White) {
		friendlyOccupance = WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
		opponentOccupance = BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
	}
	else if (ColorOfPiece(piece) == PieceColor::Black) {
		opponentOccupance = WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
		friendlyOccupance = BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
	}

	int rankDirection = 0;
	int fileDirection = 0;

	int pieceType = TypeOfPiece(piece);
	int minDir = ((pieceType == PieceType::Rook) || (pieceType == PieceType::Queen)) ? 1 : 5;
	int maxDir = ((pieceType == PieceType::Bishop) || (pieceType == PieceType::Queen)) ? 8 : 4;

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

		// To do: how to handle the king? appearantly it works without it, but no idea why
		for (int i = 1; i < 8; i++) {
			file += fileDirection;
			rank += rankDirection;
			if ((file > 7) || (file < 0) || (rank > 7) || (rank < 0)) break;
			int thatSquare = Square(rank, file);
			if (CheckBit(friendlyOccupance, thatSquare)) break;
			if (CheckBit(opponentOccupance, thatSquare)) {
				Moves.push_back(Move(home, thatSquare));
				break;
			}
			Moves.push_back(Move(home, thatSquare));
		}
	}

	return Moves;

}

std::vector<Move> Board::GenerateKingMoves(int home) {
	auto lookup = KingMoves[home];
	std::vector<Move> list;
	list.reserve(8);
	for (int l : lookup) {
		if (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(Turn)) continue;
		list.push_back(Move(home, l));
	}
	return list;
}

std::vector<Move> Board::GeneratePawnMoves(int home) {
	int piece = GetPieceAt(home);
	int color = ColorOfPiece(piece);
	int file = GetSquareFile(home);
	int rank = GetSquareRank(home);
	std::vector<Move> list;
	list.reserve(4);
	int target;

	if (color == PieceColor::White) {
		// 1. Can move forward? + check promotions
		target = home + 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 7) {
				Move m = Move(home, target);
				list.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				list.push_back(m1);
				list.push_back(m2);
				list.push_back(m3);
				list.push_back(m4);
			}
		}

		// 2. Can move up left (can capture?) + check promotions + check en passant
		target = home + 7;
		if ((file != 0) && ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target==EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0))) {
			if (GetSquareRank(target) != 7) {
				Move m = Move(home, target);
				list.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				list.push_back(m1);
				list.push_back(m2);
				list.push_back(m3);
				list.push_back(m4);
			}
		}

		// 3. Can move up right (can capture?) + check promotions + check en passant
		target = home + 9;
		if (file != 7) {
			if ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0)) {
				if (GetSquareRank(target) != 7) {
					Move m = Move(home, target);
					list.push_back(m);
				}
				else { // Promote
					Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
					Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
					Move m3 = Move(home, target, MoveFlag::PromotionToRook);
					Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
					list.push_back(m1);
					list.push_back(m2);
					list.push_back(m3);
					list.push_back(m4);
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 1) {
			bool free1 = GetPieceAt(home + 8) == 0;
			bool free2 = GetPieceAt(home + 16) == 0;
			if (free1 && free2) {
				Move m = Move(home, home+16, MoveFlag::EnPassant);
				list.push_back(m);
			}
		}
		return list;
	}

	if (color == PieceColor::Black) {
		// 1. Can move forward? + check promotions
		target = home - 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 0) {
				Move m = Move(home, target);
				list.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				list.push_back(m1);
				list.push_back(m2);
				list.push_back(m3);
				list.push_back(m4);
			}
		}

		// 2. Can move down right (can capture?) + check promotions + check en passant
		target = home - 7;
		if ((file != 7) && ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0))) {
			if (GetSquareRank(target) != 0) {
				Move m = Move(home, target);
				list.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				list.push_back(m1);
				list.push_back(m2);
				list.push_back(m3);
				list.push_back(m4);
			}
		}

		// 3. Can move down left (can capture?) + check promotions + check en passant
		target = home - 9;
		if (file != 0) {
			if ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0)) {
				if (GetSquareRank(target) != 0) {
					Move m = Move(home, target);
					list.push_back(m);
				}
				else { // Promote
					Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
					Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
					Move m3 = Move(home, target, MoveFlag::PromotionToRook);
					Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
					list.push_back(m1);
					list.push_back(m2);
					list.push_back(m3);
					list.push_back(m4);
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 6) {
			bool free1 = GetPieceAt(home - 8) == 0;
			bool free2 = GetPieceAt(home - 16) == 0;
			if (free1 && free2) {
				Move m = Move(home, home - 16, MoveFlag::EnPassant);
				list.push_back(m);
			}
		}
		return list;
	}

}

std::vector<Move> Board::GenerateCastlingMoves() {
	std::vector<Move> list;
	if ((Turn == Turn::White) && (WhiteRightToShortCastle)) {
		bool empty_f1 = GetPieceAt(5) == 0;
		bool empty_g1 = GetPieceAt(6) == 0;
		bool safe_e1 = CheckBit(AttackedSquares, 4) == 0;
		bool safe_f1 = CheckBit(AttackedSquares, 5) == 0;
		bool safe_g1 = CheckBit(AttackedSquares, 6) == 0;
		if (empty_f1 && empty_g1 && safe_e1 && safe_f1 && safe_g1) {
			Move m = Move(4, 6, MoveFlag::ShortCastle);
			list.push_back(m);
		}
	}
	if ((Turn == Turn::White) && (WhiteRightToLongCastle)) {
		bool empty_b1 = GetPieceAt(1) == 0;
		bool empty_c1 = GetPieceAt(2) == 0;
		bool empty_d1 = GetPieceAt(3) == 0;
		bool safe_c1 = CheckBit(AttackedSquares, 2) == 0;
		bool safe_d1 = CheckBit(AttackedSquares, 3) == 0;
		bool safe_e1 = CheckBit(AttackedSquares, 4) == 0;
		if (empty_b1 && empty_c1 && empty_d1 && safe_c1 && safe_d1 && safe_e1) {
			Move m = Move(4, 2, MoveFlag::LongCastle);
			list.push_back(m);
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToShortCastle)) {
		bool empty_f8 = GetPieceAt(61) == 0;
		bool empty_g8 = GetPieceAt(62) == 0;
		bool safe_e8 = CheckBit(AttackedSquares, 60) == 0;
		bool safe_f8 = CheckBit(AttackedSquares, 61) == 0;
		bool safe_g8 = CheckBit(AttackedSquares, 62) == 0;
		if (empty_f8 && empty_g8 && safe_e8 && safe_f8 && safe_g8) {
			Move m = Move(60, 62, MoveFlag::ShortCastle);
			list.push_back(m);
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToLongCastle)) {
		bool empty_b8 = GetPieceAt(57) == 0;
		bool empty_c8 = GetPieceAt(58) == 0;
		bool empty_d8 = GetPieceAt(59) == 0;
		bool safe_c8 = CheckBit(AttackedSquares, 58) == 0;
		bool safe_d8 = CheckBit(AttackedSquares, 59) == 0;
		bool safe_e8 = CheckBit(AttackedSquares, 60) == 0;
		if (empty_b8 && empty_c8 && empty_d8 && safe_c8 && safe_d8 && safe_e8) {
			Move m = Move(60, 58, MoveFlag::LongCastle);
			list.push_back(m);
		}
	}
	return list;
}

unsigned __int64 Board::CalculateAttackedSquares(int colorOfPieces) {
	unsigned __int64 squares = 0ULL;
	for (int i = 0; i < 64; i++) {
		int piece = GetPieceAt(i);
		if (ColorOfPiece(piece) == colorOfPieces) {
			int pieceType = TypeOfPiece(piece);
			switch (pieceType) {
			case PieceType::Pawn: {
				squares |= GeneratePawnAttacks(colorOfPieces, i);
				break;
			}
			case PieceType::Knight: {
				squares |= GenerateKnightAttacks(i);
				break;
			}
			case PieceType::Bishop: {
				squares |= GenerateBishopAttacks(colorOfPieces, i);
				break;
			}
			case PieceType::Rook: {
				squares |= GenerateRookAttacks(colorOfPieces, i);
				break;
			}
			case PieceType::Queen: {
				squares |= GenerateQueenAttacks(colorOfPieces, i);
				break;
			}
			case PieceType::King: {
				squares |= GenerateKingAttacks(i);
				break;
			}
			}
		}
	}
	return squares;

	/*
	std::vector<Move> moves = GenerateMoves();
	for (const Move& m : moves) {
		SetBitTrue(bits, m.to);
	}
	return bits; */
}

std::vector<Move> Board::GenerateLegalMoves(int side) {
	std::vector<Move> LegalMoves;
	if (State != GameState::Playing) return LegalMoves;

	std::vector<Move> PossibleMoves = GenerateMoves(side);

	for (const Move& m : PossibleMoves) {
		if (IsLegalMove(m, side)) LegalMoves.push_back(m);
	}

	return LegalMoves;
	// Filter attacked, parried squares, pins
}

std::vector<Move> Board::GenerateCaptureMoves(int side) { // + todo: check for promotions
	std::vector<Move> CaptureMoves;
	std::vector<Move> LegalMoves = GenerateLegalMoves(side);
	// todo: check en passant

	for (const Move& m : LegalMoves) {
		if (side == Side::White) {
			unsigned __int64 opponentOccupance = BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
			if (CheckBit(opponentOccupance, m.to)) CaptureMoves.push_back(m);
		}
		else {
			unsigned __int64 opponentOccupance = WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
			if (CheckBit(opponentOccupance, m.to)) CaptureMoves.push_back(m);
		}
	}
	return CaptureMoves;
}

std::vector<Move> Board::GenerateMoves(int side) {
	std::vector<Move> PossibleMoves;
	PossibleMoves.reserve(50);
	int myColor = SideToPieceColor(side);
	for (int i = 0; i < 64; i++) {
		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		std::vector<Move> moves;
		if (type == PieceType::Pawn) moves = GeneratePawnMoves(i);
		if (type == PieceType::Knight) moves = GenerateKnightMoves(i);
		if (type == PieceType::Bishop) moves = GenerateSlidingMoves(piece, i);
		if (type == PieceType::Rook) moves = GenerateSlidingMoves(piece, i);
		if (type == PieceType::Queen) moves = GenerateSlidingMoves(piece, i);
		if (type == PieceType::King) {
			moves = GenerateKingMoves(i);
			std::vector<Move> castlingMoves = GenerateCastlingMoves();
			moves.insert(moves.end(), castlingMoves.begin(), castlingMoves.end());
		}

		PossibleMoves.insert(PossibleMoves.end(), moves.begin(), moves.end());
	}
	return PossibleMoves;
}

bool Board::AreThereLegalMoves(int side) {
	bool hasMoves = false;
	int myColor = SideToPieceColor(side);
	std::vector<Move> moves;
	moves.reserve(27);
	for (int i = 0; i < 64; i++) {
		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		if (type == PieceType::Pawn) moves = GeneratePawnMoves(i);
		else if (type == PieceType::Knight) moves = GenerateKnightMoves(i);
		else if (type == PieceType::Bishop) moves = GenerateSlidingMoves(piece, i);
		else if (type == PieceType::Rook) moves = GenerateSlidingMoves(piece, i);
		else if (type == PieceType::Queen) moves = GenerateSlidingMoves(piece, i);
		else if (type == PieceType::King) {
			moves = GenerateKingMoves(i);
			std::vector<Move> castlingMoves = GenerateCastlingMoves();
			moves.insert(moves.end(), castlingMoves.begin(), castlingMoves.end());
		}

		if (moves.size() != 0) {
			for (Move m : moves) {
				if (IsLegalMove(m, side)) {
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

bool Board::IsLegalMove(Move m, int side) {

	// Backup variables
	unsigned __int64 whitePawnBits = WhitePawnBits;
	unsigned __int64 whiteKnightBits = WhiteKnightBits;
	unsigned __int64 whiteBishopBits = WhiteBishopBits;
	unsigned __int64 whiteRookBits = WhiteRookBits;
	unsigned __int64 whiteQueenBits = WhiteQueenBits;
	unsigned __int64 whiteKingBits = WhiteKingBits;
	unsigned __int64 blackPawnBits = BlackPawnBits;
	unsigned __int64 blackKnightBits = BlackKnightBits;
	unsigned __int64 blackBishopBits = BlackBishopBits;
	unsigned __int64 blackRookBits = BlackRookBits;
	unsigned __int64 blackQueenBits = BlackQueenBits;
	unsigned __int64 blackKingBits = BlackKingBits;
	int enPassantSquare = EnPassantSquare;
	bool whiteShortCastle = WhiteRightToShortCastle;
	bool whiteLongCastle = WhiteRightToLongCastle;
	bool blackShortCastle = BlackRightToShortCastle;
	bool blackLongCastle = BlackRightToLongCastle;
	int fullmoveClock = FullmoveClock;
	int halfmoveClock = HalfmoveClock;
	unsigned __int64 attackedSqaures = AttackedSquares;
	GameState state = State;

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
	State = state;
	AttackedSquares = attackedSqaures;


	return !inCheck;
	
}

Board Board::Copy() {
	return Board(*this);
}