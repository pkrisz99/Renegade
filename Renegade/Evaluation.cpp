#include "Evaluation.h"

Evaluation::Evaluation() {
	score = 0;
	move = Move(0, 0);
	nodes = 0;
	qnodes = 0;
	time = 0;
	nps = 0;
	hashfull = 0;
}

Evaluation::Evaluation(int s, Move m, int d, int sd, int n, int qn, int t, int speed, int h) {
	score = s;
	move = m;
	depth = d;
	seldepth = sd;
	nodes = n;
	time = t;
	nps = speed;
	hashfull = h;
}