#pragma once

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "Move.h"
#include "Utils.cpp"
//#include <windows.h>  // <-- sorry
using namespace std;

/*
* This is the board representation of Damnchess.
* Square 0 = A1, 1 = A2 ... 8 = B1 ... 63 = H8
*/

class Board
{
public:
	Board(string fen);

	void Draw(__int64 customBits);
	int GetPieceAt(int place);
	int GetPieceColor(int piece);
	int GetPieceType(int piece);
	void TryMove(Move move);
	Board Copy();

	std::vector<Move> GenerateLegalMoves();

	void SetBitTrue(__int64& number, __int64 place);
	void SetBitFalse(__int64& number, __int64 place);
	bool CheckBit(__int64& number, __int64 place);
	bool PushUci(string ucistr);
	void Push(Move move);
	bool IsLegalMove(Move m);
	int SquareToNum(string sq);
	std::vector<Move> GenerateMoves();
	std::vector<Move> GenerateKnightMoves(int square);
	std::vector<Move> GenerateCastlingMoves();
	void Test();
	int TurnToPieceColor(bool turn);
	int GetSquareRank(int square);
	int GetSquareFile(int square);
	std::vector<Move> GenerateRookMoves(int home);
	std::vector<Move> GenerateBishopMoves(int home);
	int Square(int rank, int file);
	std::vector<Move> GenerateQueenMoves(int home);
	std::vector<Move> GenerateKingMoves(int home);
	std::vector<Move> GeneratePawnMoves(int home);
	__int64 CalculateAttackedSquares();


	__int64 WhitePawnBits;
	__int64 WhiteKnightBits;
	__int64 WhiteBishopBits;
	__int64 WhiteRookBits;
	__int64 WhiteQueenBits;
	__int64 WhiteKingBits;
	__int64 BlackPawnBits;
	__int64 BlackKnightBits;
	__int64 BlackBishopBits;
	__int64 BlackRookBits;
	__int64 BlackQueenBits;
	__int64 BlackKingBits;

	__int64 AttackedSquares;

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

