#pragma once
#include "Board.h"
#include "Evaluation.h"
#include "Heuristics.h"
#include <string>
#include <tuple>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <random>
#include <filesystem>

class Engine
{
public:
	Engine();
	void perft(std::string fen, int depth, bool verbose);
	void perft(Board b, int depth, bool verbose);
	int perftRecursive(Board b, int depth);
	Evaluation Search(Board board, SearchParams params);
	eval SearchRecursive(Board board, int depth, int level, int alpha, int beta);
	//eval SearchQuiescence(Board board, int level, int alpha, int beta, int nodeEval);
	int StaticEvaluation(Board board, int level);
	int perft1(Board board, int depth, bool verbose);
	void Start();
	void PrintInfo(Evaluation e);
	void Play();
	void InitOpeningBook();
	std::string GetBookMove(unsigned __int64 hash);


	int EvaluatedNodes;
	int EvaluatedQuiescenceNodes;
	//std::unordered_map<unsigned __int64, eval> Hashes;
	std::vector<BookEntry> BookEntries;
	int HashSize;
	int SelDepth;
	int Depth;
	EngineSettings Settings;
	Heuristics Heuristics;
};

