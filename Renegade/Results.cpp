#include "Results.h"

Results::Results() {
	score = 0;
	pv = std::vector<Move>();
	nodes = 0;
	qnodes = 0;
	time = 0;
	nps = 0;
	hashfull = 0;
}

Results::Results(int s, std::vector<Move> p, int d, int sd, int n, int qn, int t, int speed, int h) {
	score = s;
	pv = p;
	depth = d;
	seldepth = sd;
	nodes = n;
	time = t;
	nps = speed;
	hashfull = h;
}

Move Results::BestMove() {
	if (pv.size() != 0) return pv.front();
	else return Move();
}