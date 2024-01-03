#include "Results.h"

Results::Results() {
	score = 0;
	pv = std::vector<Move>();
	time = 0;
	nps = 0;
	hashfull = 0;
}

Results::Results(int s, std::vector<Move> p, int d, SearchStatistics st, int t, int speed, int h, int pl) {
	score = s;
	pv = p;
	depth = d;
	stats = st;
	time = t;
	nps = speed;
	hashfull = h;
	ply = pl;
}

Move Results::BestMove() {
	if (pv.size() != 0) return pv.front();
	else return Move();
}