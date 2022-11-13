#include "Evaluation.h"

Evaluation::Evaluation() {
	score = 0;
	move = 0ULL;
	nodes = 0;
	time = 0;
	nps = 0;
	hashfull = 0;
}

Evaluation::Evaluation(int s, unsigned __int64 m, int d, int n, int t, int speed, int h) {
	score = s;
	move = m;
	depth = d;
	nodes = n;
	time = t;
	nps = speed;
	hashfull = h;
}