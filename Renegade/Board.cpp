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
	PastHashes = std::vector<uint64_t>();

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
	PastHashes.push_back(Hash(false));
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
	PastHashes = std::vector<uint64_t>(b.PastHashes.begin(), b.PastHashes.end());
}

void Board::Draw(uint64_t customBits = 0) {

	string side = Turn ? "white" : "black";
	cout << "    Move: " << FullmoveClock << " - " << side << " to play" << endl;;
	
	const string WhiteOnLightSquare = "\033[31;47m";
	const string WhiteOnDarkSquare = "\033[31;43m";
	const string BlackOnLightSquare = "\033[30;47m";
	const string BlackOnDarkSquare = "\033[30;43m";
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
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnLightSquare;
				else CellStyle = WhiteOnLightSquare;
			}
			else {
				if (pieceColor == PieceColor::Black) CellStyle = BlackOnDarkSquare;
				else CellStyle = WhiteOnDarkSquare;
			}

			if (CheckBit(customBits, (uint64_t)i * 8 + j)) {
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

uint64_t Board::Hash(bool hashPlys) {
	uint64_t hash = 0ULL;

	// Pieces
	uint64_t occupancy = GetOccupancy();
	while (NonZeros(occupancy) != 0) {
		uint64_t sq = 64 - __lzcnt64(occupancy) - 1;
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

	// Ply number - internal only
	if (hashPlys) hash ^= (FullmoveClock * 0xE0C754AULL);

	return hash;
}

int Board::GetPieceAt(int place) {
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

uint64_t Board::GetOccupancy() {
	return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits
		| BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
}

uint64_t Board::GetOccupancy(int pieceColor) {
	if (pieceColor == PieceColor::White) return WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
	return BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
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
		SetBitFalse(BlackPawnBits, (uint64_t)(EnPassantSquare)-8);
	}
	if ((piece == Piece::BlackPawn) && (move.to == EnPassantSquare)) {
		SetBitFalse(WhitePawnBits, (uint64_t)(EnPassantSquare)+8);
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

	if (move.flag != MoveFlag::NullMove) TryMove(move);

	// Increment timers
	Turn = !Turn;
	HalfmoveClock += 1;
	if (Turn == Turn::White) FullmoveClock += 1;

	const uint64_t previousAttackMap = AttackedSquares;
	AttackedSquares = CalculateAttackedSquares(TurnToPieceColor(!Turn));

	// Get state after
	int inCheck = 0;
	if (Turn == Turn::White) inCheck = NonZeros(AttackedSquares & WhiteKingBits);
	else inCheck = NonZeros(AttackedSquares & BlackKingBits);

	// Add current hash to list
	uint64_t hash = Hash(false);
	PastHashes.push_back(hash);
	
	// Check checkmates & stalemates
	const bool hasMoves = AreThereLegalMoves(Turn, previousAttackMap);
	if (!hasMoves) {
		if (inCheck) {
			if (Turn == Turn::Black) State = GameState::WhiteVictory;
			else State = GameState::BlackVictory;
		} else {
			State = GameState::Draw;
		}
	}
	if ((State != GameState::Playing) || (!DrawCheck)) return;

	// Threefold repetition check
	// optimalization idea: only check the last 'HalfmoveClock' moves?
	if (DrawCheck) {
		int64_t stateCount = std::count(PastHashes.begin(), PastHashes.end(), hash);
		if (stateCount >= 3) {
			State = GameState::Draw;
			return;
		}
	}


	// Insufficient material check
	const int WhitePawnCount = NonZeros(WhitePawnBits);
	const int WhiteKnightCount = NonZeros(WhiteKnightBits);
	const int WhiteBishopCount = NonZeros(WhiteBishopBits);
	const int WhiteRookCount = NonZeros(WhiteRookBits);
	const int WhiteQueenCount = NonZeros(WhiteQueenBits);
	const int BlackPawnCount = NonZeros(BlackPawnBits);
	const int BlackKnightCount = NonZeros(BlackKnightBits);
	const int BlackBishopCount = NonZeros(BlackBishopBits);
	const int BlackRookCount = NonZeros(BlackRookBits);
	const int BlackQueenCount = NonZeros(BlackQueenBits);
	const int WhitePieceCount = WhitePawnCount + WhiteKnightCount + WhiteBishopCount + WhiteRookCount + WhiteQueenCount;
	const int BlackPieceCount = BlackPawnCount + BlackKnightCount + BlackBishopCount + BlackRookCount + BlackQueenCount;
	const int WhiteLightBishopCount = NonZeros(WhiteBishopBits & Bitboards::LightSquares);
	const int WhiteDarkBishopCount = NonZeros(WhiteBishopBits & Bitboards::DarkSquares);
	const int BlackLightBishopCount = NonZeros(BlackBishopBits & Bitboards::LightSquares);
	const int BlackDarkBishopCount = NonZeros(BlackBishopBits & Bitboards::DarkSquares);

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
	if (HalfmoveClock > 100) State = GameState::Draw;

}

uint64_t Board::GenerateKnightAttacks(int from) {
	return KnightMoveBits[from];
}

uint64_t Board::GenerateKingAttacks(int from) {
	return KingMoveBits[from];
}


void Board::GenerateKnightMoves(int home) {
	auto lookup = KnightMoves[home];
	for (int l : lookup) {
		if (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(Turn)) continue;
		MoveList.push_back(Move(home, l));
	}
}

void Board::GenerateSlidingMoves(int piece, int home) {
	uint64_t friendlyOccupance = 0ULL;
	uint64_t opponentOccupance = 0ULL;
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

		// To do: how to handle the king? appearantly it works without it, but no idea why
		for (int i = 1; i < 8; i++) {
			file += fileDirection;
			rank += rankDirection;
			if ((file > 7) || (file < 0) || (rank > 7) || (rank < 0)) break;
			int thatSquare = Square(rank, file);
			if (CheckBit(friendlyOccupance, thatSquare)) break;
			if (CheckBit(opponentOccupance, thatSquare)) {
				MoveList.push_back(Move(home, thatSquare));
				break;
			}
			MoveList.push_back(Move(home, thatSquare));
		}
	}

}

inline uint64_t Board::GenerateSlidingAttacksShiftDown(int direction, uint64_t boundMask, uint64_t propagatingPieces, uint64_t friendlyPieces, uint64_t opponentPieces) {
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

inline uint64_t Board::GenerateSlidingAttacksShiftUp(int direction, uint64_t boundMask, uint64_t propagatingPieces, uint64_t friendlyPieces, uint64_t opponentPieces) {
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

void Board::GenerateKingMoves(int home) {
	const auto lookup = KingMoves[home];
	for (const int l : lookup) {
		if (ColorOfPiece(GetPieceAt(l)) == TurnToPieceColor(Turn)) continue;
		MoveList.push_back(Move(home, l));
	}

}

void Board::GeneratePawnMoves(int home) {
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
				Move m = Move(home, target);
				MoveList.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				MoveList.push_back(m4);
				MoveList.push_back(m3);
				MoveList.push_back(m2);
				MoveList.push_back(m1);
			}
		}

		// 2. Can move up left (can capture?) + check promotions + check en passant
		target = home + 7;
		if ((file != 0) && ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target==EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0))) {
			if (GetSquareRank(target) != 7) {
				Move m = Move(home, target);
				MoveList.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				MoveList.push_back(m4);
				MoveList.push_back(m3);
				MoveList.push_back(m2);
				MoveList.push_back(m1);
			}
		}

		// 3. Can move up right (can capture?) + check promotions + check en passant
		target = home + 9;
		if (file != 7) {
			if ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0)) {
				if (GetSquareRank(target) != 7) {
					Move m = Move(home, target);
					MoveList.push_back(m);
				}
				else { // Promote
					Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
					Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
					Move m3 = Move(home, target, MoveFlag::PromotionToRook);
					Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
					MoveList.push_back(m4);
					MoveList.push_back(m3);
					MoveList.push_back(m2);
					MoveList.push_back(m1);
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 1) {
			bool free1 = GetPieceAt(home + 8) == 0;
			bool free2 = GetPieceAt(home + 16) == 0;
			if (free1 && free2) {
				Move m = Move(home, home+16, MoveFlag::EnPassant);
				MoveList.push_back(m);
			}
		}
	}

	if (color == PieceColor::Black) {
		// 1. Can move forward? + check promotions
		target = home - 8;
		if (GetPieceAt(target) == 0) {
			if (GetSquareRank(target) != 0) {
				Move m = Move(home, target);
				MoveList.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				MoveList.push_back(m4);
				MoveList.push_back(m3);
				MoveList.push_back(m2);
				MoveList.push_back(m1);
			}
		}

		// 2. Can move down right (can capture?) + check promotions + check en passant
		target = home - 7;
		if ((file != 7) && ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0))) {
			if (GetSquareRank(target) != 0) {
				Move m = Move(home, target);
				MoveList.push_back(m);
			} else { // Promote
				Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
				Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
				Move m3 = Move(home, target, MoveFlag::PromotionToRook);
				Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
				MoveList.push_back(m4);
				MoveList.push_back(m3);
				MoveList.push_back(m2);
				MoveList.push_back(m1);
			}
		}

		// 3. Can move down left (can capture?) + check promotions + check en passant
		target = home - 9;
		if (file != 0) {
			if ((ColorOfPiece(GetPieceAt(target)) == TurnToPieceColor(!Turn)) || (target == EnPassantSquare) && (ColorOfPiece(GetPieceAt(target)) == 0)) {
				if (GetSquareRank(target) != 0) {
					Move m = Move(home, target);
					MoveList.push_back(m);
				}
				else { // Promote
					Move m1 = Move(home, target, MoveFlag::PromotionToKnight);
					Move m2 = Move(home, target, MoveFlag::PromotionToBishop);
					Move m3 = Move(home, target, MoveFlag::PromotionToRook);
					Move m4 = Move(home, target, MoveFlag::PromotionToQueen);
					MoveList.push_back(m4);
					MoveList.push_back(m3);
					MoveList.push_back(m2);
					MoveList.push_back(m1);
				}
			}
		}

		// 4. Can move double (can't skip)
		if (rank == 6) {
			bool free1 = GetPieceAt(home - 8) == 0;
			bool free2 = GetPieceAt(home - 16) == 0;
			if (free1 && free2) {
				Move m = Move(home, home - 16, MoveFlag::EnPassant);
				MoveList.push_back(m);
			}
		}
	}
}

void Board::GenerateCastlingMoves() {
	if ((Turn == Turn::White) && (WhiteRightToShortCastle)) {
		bool empty_f1 = GetPieceAt(Squares::F1) == 0;
		bool empty_g1 = GetPieceAt(Squares::G1) == 0;
		bool safe_e1 = CheckBit(AttackedSquares, Squares::E1) == 0;
		bool safe_f1 = CheckBit(AttackedSquares, Squares::F1) == 0;
		bool safe_g1 = CheckBit(AttackedSquares, Squares::G1) == 0;
		if (empty_f1 && empty_g1 && safe_e1 && safe_f1 && safe_g1) {
			Move m = Move(Squares::E1, Squares::G1, MoveFlag::ShortCastle);
			MoveList.push_back(m);
		}
	}
	if ((Turn == Turn::White) && (WhiteRightToLongCastle)) {
		bool empty_b1 = GetPieceAt(Squares::B1) == 0;
		bool empty_c1 = GetPieceAt(Squares::C1) == 0;
		bool empty_d1 = GetPieceAt(Squares::D1) == 0;
		bool safe_c1 = CheckBit(AttackedSquares, Squares::C1) == 0;
		bool safe_d1 = CheckBit(AttackedSquares, Squares::D1) == 0;
		bool safe_e1 = CheckBit(AttackedSquares, Squares::E1) == 0;
		if (empty_b1 && empty_c1 && empty_d1 && safe_c1 && safe_d1 && safe_e1) {
			Move m = Move(Squares::E1, Squares::C1, MoveFlag::LongCastle);
			MoveList.push_back(m);
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToShortCastle)) {
		bool empty_f8 = GetPieceAt(Squares::F8) == 0;
		bool empty_g8 = GetPieceAt(Squares::G8) == 0;
		bool safe_e8 = CheckBit(AttackedSquares, Squares::E8) == 0;
		bool safe_f8 = CheckBit(AttackedSquares, Squares::F8) == 0;
		bool safe_g8 = CheckBit(AttackedSquares, Squares::G8) == 0;
		if (empty_f8 && empty_g8 && safe_e8 && safe_f8 && safe_g8) {
			Move m = Move(60, 62, MoveFlag::ShortCastle);
			MoveList.push_back(m);
		}
	}
	if ((Turn == Turn::Black) && (BlackRightToLongCastle)) {
		bool empty_b8 = GetPieceAt(Squares::B8) == 0;
		bool empty_c8 = GetPieceAt(Squares::C8) == 0;
		bool empty_d8 = GetPieceAt(Squares::D8) == 0;
		bool safe_c8 = CheckBit(AttackedSquares, Squares::C8) == 0;
		bool safe_d8 = CheckBit(AttackedSquares, Squares::D8) == 0;
		bool safe_e8 = CheckBit(AttackedSquares, Squares::E8) == 0;
		if (empty_b8 && empty_c8 && empty_d8 && safe_c8 && safe_d8 && safe_e8) {
			Move m = Move(Squares::E8, Squares::C8, MoveFlag::LongCastle);
			MoveList.push_back(m);
		}
	}
}

uint64_t Board::CalculateAttackedSquares(int colorOfPieces) {
	uint64_t squares = 0ULL;
	uint64_t parallelSliders = 0;
	uint64_t diagonalSliders = 0;
	uint64_t friendlyPieces = 0;
	uint64_t opponentPieces = 0;

	if (colorOfPieces == PieceColor::White) {
		parallelSliders = WhiteRookBits | WhiteQueenBits;
		diagonalSliders = WhiteBishopBits | WhiteQueenBits;
		friendlyPieces = WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
		opponentPieces = BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
	}
	else {
		parallelSliders = BlackRookBits | BlackQueenBits;
		diagonalSliders = BlackBishopBits | BlackQueenBits;
		opponentPieces = WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
		friendlyPieces = BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
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
	uint64_t kingBits = colorOfPieces == PieceColor::White ? WhiteKingBits : BlackKingBits;
	uint64_t fill = 0;
	fill |= (kingBits & ~Bitboards::FileH) << 1;
	fill |= (kingBits & ~Bitboards::FileA) >> 1;
	fill |= (kingBits & ~Bitboards::Rank8) << 8;
	fill |= (kingBits & ~Bitboards::Rank1) >> 8;
	fill |= (kingBits & ~Bitboards::FileA & ~Bitboards::Rank8) << 7;
	fill |= (kingBits & ~Bitboards::FileH & ~Bitboards::Rank8) << 9;
	fill |= (kingBits & ~Bitboards::FileH & ~Bitboards::Rank1) >> 7;
	fill |= (kingBits & ~Bitboards::FileA & ~Bitboards::Rank1) >> 9;
	fill = fill & ~friendlyPieces;
	squares |= fill;

	// Knight attack gen: +6, +10, +15, +17
	uint64_t knightBits = colorOfPieces == PieceColor::White ? WhiteKnightBits : BlackKnightBits;
	fill = 0;
	while (NonZeros(knightBits) != 0) {
		uint64_t sq = 64 - __lzcnt64(knightBits) - 1;
		fill |= GenerateKnightAttacks((int)sq);
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
	std::vector<Move> PseudoLegalMoves = GenerateMoves(side);
	// todo: check en passant + extend for promotions

	for (const Move& m : PseudoLegalMoves) {
		if (side == Side::White) {
			uint64_t opponentOccupance = BlackPawnBits | BlackKnightBits | BlackBishopBits | BlackRookBits | BlackQueenBits | BlackKingBits;
			if (CheckBit(opponentOccupance, m.to)) CaptureMoves.push_back(m);
		}
		else if (side == Side::Black) {
			uint64_t opponentOccupance = WhitePawnBits | WhiteKnightBits | WhiteBishopBits | WhiteRookBits | WhiteQueenBits | WhiteKingBits;
			if (CheckBit(opponentOccupance, m.to)) CaptureMoves.push_back(m);
		}
	}

	return CaptureMoves;
}

std::vector<Move> Board::GenerateMoves(int side) {
	//std::vector<Move> PossibleMoves;
	MoveList.clear();
	MoveList.reserve(10);
	int myColor = SideToPieceColor(side);
	uint64_t occupancy = GetOccupancy(SideToPieceColor(side));
	while (occupancy != 0) {
		uint64_t i = 64 - __lzcnt64(occupancy) - 1;
		SetBitFalse(occupancy, i);

		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		std::vector<Move> moves;
		if (type == PieceType::Pawn) GeneratePawnMoves(i);
		else if (type == PieceType::Knight) GenerateKnightMoves(i);
		else if (type == PieceType::Bishop) GenerateSlidingMoves(piece, i);
		else if (type == PieceType::Rook) GenerateSlidingMoves(piece, i);
		else if (type == PieceType::Queen) GenerateSlidingMoves(piece, i);
		else if (type == PieceType::King) {
			GenerateKingMoves(i);
			GenerateCastlingMoves();
		}
	}
	return MoveList;
}

bool Board::AreThereLegalMoves(int side, uint64_t previousAttackMap) {  
	MoveList.clear();
	MoveList.reserve(10);
	bool hasMoves = false;
	int myColor = SideToPieceColor(side);

	// Quick king test - if the king can move to a free square without being attacked, then we have a legal move
	uint64_t kingBits = side == Side::White ? WhiteKingBits : BlackKingBits;
	uint64_t sq = 64 - __lzcnt64(kingBits) - 1;
	uint64_t kingMoveBits = GenerateKingAttacks((int)sq);
	uint64_t fastKingCheck = (~previousAttackMap) & (~GetOccupancy()) & kingMoveBits;
	if (fastKingCheck != 0) return true;

	uint64_t occupancy = GetOccupancy(SideToPieceColor(side));
	while (occupancy != 0) {
		uint64_t i = 64 - __lzcnt64(occupancy) - 1;
		SetBitFalse(occupancy, i);
		int piece = GetPieceAt(i);
		int color = ColorOfPiece(piece);
		int type = TypeOfPiece(piece);
		if (color != myColor) continue;

		if (type == PieceType::Pawn) GeneratePawnMoves(i);
		else if (type == PieceType::Knight) GenerateKnightMoves(i);
		else if (type == PieceType::Bishop) GenerateSlidingMoves(piece, i);
		else if (type == PieceType::Rook) GenerateSlidingMoves(piece, i);
		else if (type == PieceType::Queen) GenerateSlidingMoves(piece, i);
		else if (type == PieceType::King) {
			GenerateKingMoves(i);
			GenerateCastlingMoves();
		}

		if (MoveList.size() != 0) {
			for (Move m : MoveList) {
				if (IsLegalMove(m, side)) {
					MoveList.clear();
					hasMoves = true;
					break;
				}
			}
			MoveList.clear();
		}

		if (hasMoves) break;
	}
	return hasMoves;
}

bool Board::IsLegalMove(Move m, int side) {

	// Backup variables
	uint64_t whitePawnBits = WhitePawnBits;
	uint64_t whiteKnightBits = WhiteKnightBits;
	uint64_t whiteBishopBits = WhiteBishopBits;
	uint64_t whiteRookBits = WhiteRookBits;
	uint64_t whiteQueenBits = WhiteQueenBits;
	uint64_t whiteKingBits = WhiteKingBits;
	uint64_t blackPawnBits = BlackPawnBits;
	uint64_t blackKnightBits = BlackKnightBits;
	uint64_t blackBishopBits = BlackBishopBits;
	uint64_t blackRookBits = BlackRookBits;
	uint64_t blackQueenBits = BlackQueenBits;
	uint64_t blackKingBits = BlackKingBits;
	int enPassantSquare = EnPassantSquare;
	bool whiteShortCastle = WhiteRightToShortCastle;
	bool whiteLongCastle = WhiteRightToLongCastle;
	bool blackShortCastle = BlackRightToShortCastle;
	bool blackLongCastle = BlackRightToLongCastle;
	int fullmoveClock = FullmoveClock;
	int halfmoveClock = HalfmoveClock;
	uint64_t attackedSquares = AttackedSquares;
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
	AttackedSquares = attackedSquares;


	return !inCheck;
	
}

Board Board::Copy() {
	return Board(*this);
}