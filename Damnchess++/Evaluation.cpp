#include "Evaluation.h"

Evaluation::Evaluation(int s, Move m, int n, int t, int speed) {
	score = s;
	move = m;
	nodes = n;
	time = t;
	nps = speed;
}