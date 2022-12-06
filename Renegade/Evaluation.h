#pragma once
#include "Move.h"

static const int MateEval = 1000000;
static const int NoEval = -666666666;
static const int NegativeInfinity = -333333333; // Inventing a new kind of math here
static const int PositiveInfinity = 444444444; // These numbers are easy to recognize if something goes wrong


struct eval {
	int score;
	std::vector<Move> moves;

	eval(int s);
	eval(int s, std::vector<Move> m);
	eval();
};

//typedef std::tuple<int, Move> eval;

static const int PawnValue = 100;
static const int KnightValue = 300;
static const int BishopValue = 300;
static const int RookValue = 500;
static const int QueenValue = 900;

static const int PawnPSQT[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
	10,  10,  15,  20,  20,  15,  10,  10,
	20,  25,  30,  35,  35,  30,  25,  20,
	40,  45,  50,  60,  60,  50,  45,  40,
	70,  80,  80,  80,  80,  80,  80,  70,
   140, 150, 160, 160, 160, 160, 150, 140,
	 0,   0,   0,   0,   0,   0,   0,   0 };

static const int KnightPSQT[64] = {
	-50, -40, -30, -30, -30, -30, -40, -50,
	-40, -20,   0,   5,   5,   0, -20, -40,
	-30,   5,  10,  15,  15,  10,   5, -30,
	-30,   0,  15,  20,  20,  15,   0, -30,
	-30,   5,  15,  20,  20,  15,   5, -30,
	-30,   0,  10,  15,  15,  10,   0, -30,
	-40, -20,   0,   0,   0,   0, -20, -40,
	-50, -40, -30, -30, -30, -30, -40, -50
};

static const int BishopPSQT[64] = {
	-20, -10, -10, -10, -10, -10, -10, -20,
	-10,   5,   0,   0,   0,   0,   5, -10,
	-10,  10,  10,  10,  10,  10,  10, -10,
	-10,   0,  10,  10,  10,  10,   0, -10,
	-10,   5,   5,  10,  10,   5,   5, -10,
	-10,   0,   5,  10,  10,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-20, -10, -10, -10, -10, -10, -10, -20
};

static const int RookPSQT[64] = {
	 0,  0,  0,  5,  5,  0,  0,  0,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	-5,  0,  0,  0,  0,  0,  0, -5,
	 5, 10, 10, 10, 10, 10, 10,  5,
	 0,  0,  0,  0,  0,  0,  0,  0
};

static const int QueenPSQT[64] = {
	-20, -10, -10,  -5,  -5, -10, -10, -20,
	-10,   0,   5,   0,   0,   0,   0, -10,
	-10,   5,   5,   5,   5,   5,   0, -10,
	  0,   0,   5,   5,   5,   5,   0,  -5,
	 -5,   0,   5,   5,   5,   5,   0,  -5,
	-10,   0,   5,   5,   5,   5,   0, -10,
	-10,   0,   0,   0,   0,   0,   0, -10,
	-20, -10, -10,  -5,  -5, -10, -10, -20
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

