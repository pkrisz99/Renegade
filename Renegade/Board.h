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
	void Draw(uint64_t customBits);
	int GetPieceAt(int place);
	uint64_t Hash(bool hashPlys);
	uint64_t GetOccupancy();
	uint64_t GetOccupancy(int pieceColor);

	bool AreThereLegalMoves(int turn, uint64_t previousAttackMap);
	bool IsLegalMove(Move m, int turn);
	void TryMove(Move move);
	vector<Move> GenerateMoves(int turn);
	vector<Move> GenerateLegalMoves(int turn);
	vector<Move> GenerateCaptureMoves(int turn);
	uint64_t CalculateAttackedSquares(int turn);

	void GenerateKnightMoves(int home);
	void GenerateKingMoves(int home);
	void GeneratePawnMoves(int home);
	void GenerateCastlingMoves();
	void GenerateSlidingMoves(int piece, int home);

	uint64_t GenerateKnightAttacks(int from);
	uint64_t GenerateKingAttacks(int from);
	uint64_t GenerateSlidingAttacksShiftUp(int direction, uint64_t boundMask, uint64_t propagatingPieces,
		uint64_t friendlyPieces, uint64_t opponentPieces);
	uint64_t GenerateSlidingAttacksShiftDown(int direction, uint64_t boundMask, uint64_t propagatingPieces,
		uint64_t friendlyPieces, uint64_t opponentPieces);

	// Board variables:
	uint64_t WhitePawnBits;
	uint64_t WhiteKnightBits;
	uint64_t WhiteBishopBits;
	uint64_t WhiteRookBits;
	uint64_t WhiteQueenBits;
	uint64_t WhiteKingBits;
	uint64_t BlackPawnBits;
	uint64_t BlackKnightBits;
	uint64_t BlackBishopBits;
	uint64_t BlackRookBits;
	uint64_t BlackQueenBits;
	uint64_t BlackKingBits;
	uint64_t AttackedSquares;
	int EnPassantSquare;
	bool WhiteRightToShortCastle;
	bool WhiteRightToLongCastle;
	bool BlackRightToShortCastle;
	bool BlackRightToLongCastle;
	bool Turn;
	int HalfmoveClock;
	int FullmoveClock;
	GameState State;
	std::vector<uint64_t> PastHashes;
	std::vector<Move> MoveList;

	// Board settings:
	bool StartingPosition;
	bool DrawCheck = true;

};

