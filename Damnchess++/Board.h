#pragma once

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "Move.h"
#include "Utils.cpp"
using namespace std;

/*
* This is the board representation of Damnchess.
* Square 0 = A1, 1 = A2 ... 8 = B1 ... 63 = H8
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

	bool IsLegalMove(Move m, int side);
	void TryMove(Move move);
	std::vector<Move> GenerateMoves(int side);
	std::vector<Move> GenerateLegalMoves(int side);
	unsigned __int64 CalculateAttackedSquares(int side);

	std::vector<Move> GenerateKnightMoves(int home);
	std::vector<Move> GenerateRookMoves(int home);
	std::vector<Move> GenerateBishopMoves(int home);
	std::vector<Move> GenerateQueenMoves(int home);
	std::vector<Move> GenerateKingMoves(int home);
	std::vector<Move> GeneratePawnMoves(int home);
	std::vector<Move> GenerateCastlingMoves();

	unsigned __int64 GenerateKnightAttacks(int from);
	unsigned __int64 GenerateKingAttacks(int from);
	unsigned __int64 GenerateBishopAttacks(int pieceColor, int home);
	unsigned __int64 GenerateRookAttacks(int pieceColor, int from);
	unsigned __int64 GenerateQueenAttacks(int pieceColor, int from);
	unsigned __int64 GeneratePawnAttacks(int pieceColor, int from);


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

	bool DrawCheck = true;

	int HalfmoveClock;
	int FullmoveClock;

	GameState State;

};

