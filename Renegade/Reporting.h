#pragma once

#include "Move.h"
#include "Settings.h"

struct Results {
	int score = 0;
	int depth = 0;
	int seldepth = 0;
	uint64_t nodes = 0;
	int time = 0;
	int nps = 0;
	int hashfull = 0;
	int ply = 0;
	std::vector<Move> pv;
	int threads = 1;
	
	int blendedScore = NoEval;

	Results();
	Results(const int score, const int depth, const int seldepth, const uint64_t nodes, const int time,
		const int nps, const int hashfull, const int ply, const std::vector<Move>& pv);

	inline void UpdateBlendedScore(const int score) {
		blendedScore = [&] {
			if (std::abs(score) >= MateThreshold) return score;
			if (blendedScore == NoEval) return score;
			return (score * 2 + blendedScore) / 3;
		}();
	}


	Move BestMove() const;
};

void PrintInfo(const Results& e);
void PrintPretty(const Results& e);
void PrintBestmove(const Move& move);
