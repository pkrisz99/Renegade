#include "Evaluation.h"

Evaluation::Evaluation() {
	score = 0;
	move = Move(0, 0);
	nodes = 0;
	time = 0;
	nps = 0;
}

Evaluation::Evaluation(int s, Move m, int d, int n, int t, int speed) {
	score = s;
	move = m;
	depth = d;
	nodes = n;
	time = t;
	nps = speed;
}