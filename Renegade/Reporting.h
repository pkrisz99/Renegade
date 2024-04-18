#pragma once

#include "Move.h"
#include "Settings.h"

class Results {
public:
	int score = 0;
	std::vector<Move> pv;
	int depth = 0;
	SearchStatistics stats;
	int time = 0;
	int nps = 0;
	int hashfull = 0;
	int ply = 0;
	Results();
	Results(int s, std::vector<Move> p, int d, SearchStatistics stats, int t, int speed, int h, int pl);
	Move BestMove();
};

void PrintInfo(const Results& e);
void PrintPretty(const Results& e);
void PrintBestmove(const Move& move);
