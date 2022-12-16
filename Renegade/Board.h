#pragma once

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "Move.h"
#include "Utils.cpp"
#include <intrin.h>

using std::cout;
using std::endl;
using std::cin;
using std::string;
using std::vector;
using std::get;

/*
* This is the board representation of Renegade.
* Square 0 = A1, 1 = B1 ... 8 = A2 ... 63 = H8
*/

class Board
{

public:
	Board();
	Board(const Board &b);
	Board(string fen);
	void Push(Move move);
	bool PushUci(string ucistr);
	Board Copy();
	void Draw(unsigned __int64 customBits);
	int GetPieceAt(int place);
	unsigned __int64 Hash(bool hashPlys);

	bool AreThereLegalMoves(int side, uint64_t lastAttackMap);
	bool IsLegalMove(Move m, int side);
	void TryMove(Move move);
	vector<Move> GenerateMoves(int side);
	vector<Move> GenerateLegalMoves(int side);
	vector<Move> GenerateCaptureMoves(int side);
	unsigned __int64 CalculateAttackedSquares(int side);

	std::vector<Move> GenerateKnightMoves(int home);
	std::vector<Move> GenerateKingMoves(int home);
	std::vector<Move> GeneratePawnMoves(int home);
	std::vector<Move> GenerateCastlingMoves();
	std::vector<Move> GenerateSlidingMoves(int piece, int home);

	unsigned __int64 GenerateKnightAttacks(int from);
	unsigned __int64 GenerateKingAttacks(int from);
	unsigned __int64 GeneratePawnAttacks(int pieceColor, int from);
	unsigned __int64 GenerateSlidingAttacksShiftUp(int direction, unsigned __int64 boundMask, unsigned __int64 propagatingPieces,
		unsigned __int64 friendlyPieces, unsigned __int64 opponentPieces);
	unsigned __int64 GenerateSlidingAttacksShiftDown(int direction, unsigned __int64 boundMask, unsigned __int64 propagatingPieces,
		unsigned __int64 friendlyPieces, unsigned __int64 opponentPieces);

	// Board variables:
	unsigned __int64 WhitePawnBits;
	unsigned __int64 WhiteKnightBits;
	unsigned __int64 WhiteBishopBits;
	unsigned __int64 WhiteRookBits;
	unsigned __int64 WhiteQueenBits;
	unsigned __int64 WhiteKingBits;
	unsigned __int64 BlackPawnBits;
	unsigned __int64 BlackKnightBits;
	unsigned __int64 BlackBishopBits;
	unsigned __int64 BlackRookBits;
	unsigned __int64 BlackQueenBits;
	unsigned __int64 BlackKingBits;
	unsigned __int64 AttackedSquares;
	int EnPassantSquare;
	bool WhiteRightToShortCastle;
	bool WhiteRightToLongCastle;
	bool BlackRightToShortCastle;
	bool BlackRightToLongCastle;
	bool Turn;
	int HalfmoveClock;
	int FullmoveClock;
	GameState State;
	std::vector<unsigned __int64> PastHashes;

	// Board settings:
	bool StartingPosition;
	bool DrawCheck = true;

};

