#pragma once
#include "Move.h"
#include "Board.h"

static const int MateEval = 1000000;
static const int NoEval = -666666666;
static const int NegativeInfinity = -333333333; // Inventing a new kind of math here
static const int PositiveInfinity = 444444444; // These numbers are easy to recognize if something goes wrong

// Source of values:
// https://www.chessprogramming.org/Simplified_Evaluation_Function

const class Weights {
public:

	static const int PawnValue = 100;
	static const int KnightValue = 300;
	static const int BishopValue = 300;
	static const int RookValue = 500;
	static const int QueenValue = 900;
	static constexpr int PieceMaterial[] = { 0, PawnValue, KnightValue, BishopValue, RookValue, QueenValue, 0 };

	static const int BishopPairBonus = 50;

	static constexpr int PawnPSQT[64] = {
		 0,   0,   0,   0,   0,   0,   0,   0,
		 0,   0,   0,   0,   0,   0,   0,   0,
		10,  10,  15,  20,  20,  15,  10,  10,
		20,  25,  30,  35,  35,  30,  25,  20,
		40,  45,  50,  60,  60,  50,  45,  40,
		70,  80,  80,  80,  80,  80,  80,  70,
	   140, 150, 160, 160, 160, 160, 150, 140,
		 0,   0,   0,   0,   0,   0,   0,   0 };

	static constexpr int KnightPSQT[64] = {
		-50, -40, -30, -30, -30, -30, -40, -50,
		-40, -20,   0,   5,   5,   0, -20, -40,
		-30,   5,  10,  15,  15,  10,   5, -30,
		-30,   0,  15,  20,  20,  15,   0, -30,
		-30,   5,  15,  20,  20,  15,   5, -30,
		-30,   0,  10,  15,  15,  10,   0, -30,
		-40, -20,   0,   0,   0,   0, -20, -40,
		-50, -40, -30, -30, -30, -30, -40, -50
	};

	static constexpr int BishopPSQT[64] = {
		-20, -10, -10, -10, -10, -10, -10, -20,
		-10,   5,   0,   0,   0,   0,   5, -10,
		-10,  10,  10,  10,  10,  10,  10, -10,
		-10,   0,  10,  10,  10,  10,   0, -10,
		-10,   5,   5,  10,  10,   5,   5, -10,
		-10,   0,   5,  10,  10,   5,   0, -10,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-20, -10, -10, -10, -10, -10, -10, -20
	};

	static constexpr int RookPSQT[64] = {
		 0,  0,  0,  5,  5,  0,  0,  0,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		 5, 10, 10, 10, 10, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};

	static constexpr int QueenPSQT[64] = {
		-20, -10, -10,  -5,  -5, -10, -10, -20,
		-10,   0,   5,   0,   0,   0,   0, -10,
		-10,   5,   5,   5,   5,   5,   0, -10,
		  0,   0,   5,   5,   5,   5,   0,  -5,
		 -5,   0,   5,   5,   5,   5,   0,  -5,
		-10,   0,   5,   5,   5,   5,   0, -10,
		-10,   0,   0,   0,   0,   0,   0, -10,
		-20, -10, -10,  -5,  -5, -10, -10, -20
	};

	static constexpr int KingEarlyPSQT[64] = {
		 20,  30,  10,   0,   0,  10,  30,  20,
		 20,  20,   0,   0,   0,   0,  20,  20,
		-10, -20, -20, -20, -20, -20, -20, -10,
		-20, -30, -30, -40, -40, -30, -30, -20,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30,
		-30, -40, -40, -50, -50, -40, -40, -30
	};

	static constexpr int KingLatePSQT[64] = {
		-50, -30, -30, -30, -30, -30, -30, -50,
		-30, -30,   0,   0,   0,   0, -30, -30,
		-30, -10,  20,  30,  30,  20, -10, -30,
		-30, -10,  30,  40,  40,  30, -10, -30,
		-30, -10,  30,  40,  40,  30, -10, -30,
		-30, -10,  20,  30,  30,  20, -10, -30,
		-30, -20, -10,   0,   0, -10, -20, -30,
		-50, -40, -30, -20, -20, -30, -40, -50
	};

};

class Evaluation
{
public:
	int score = 0;
	std::vector<Move> pv;
	int depth = 0;
	int seldepth = 0;
	int nodes = 0;
	int qnodes = 0;
	int time = 0;
	int nps = 0;
	int hashfull = 0;
	Evaluation();
	Evaluation(int s, std::vector<Move> p, int d, int sd, int n, int qn, int t, int speed, int h);
	Move BestMove();
};

static const int GetTaperedPSQT(const int earlyValue, const int lateValue, const float phase) {
	return (int)((1 - phase) * (float)earlyValue + phase * (float)lateValue);
}

static const int CalculateGamePhase(Board &board) {
	int remainingPieces = Popcount(board.GetOccupancy());
	float phase = (32.f - remainingPieces) / (32.f - 4.f);
	return std::clamp(phase, 0.f, 1.f);
}