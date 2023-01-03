#pragma once
#include <iostream>
#include <fstream>
#include "Board.h"
#include "Evaluation.h"
#include "Utils.cpp"

// This is the header file of Renegade's tuner. It is based on the Texel tuning method, and runs until no improvement can be found.
// It is almost like a single layer neural network. Uses separate training and test datasets.

// For the test run of 2.43 million positions on a ~5 year old laptop:
// - About 1 GB RAM needed, loads quite quickly
// - K determination takes about 90 seconds
// - Iterations take roughly 15 minutes each.

class Tuning
{
public:
	class TempWeights {
	public:
		int PawnValue;
		int KnightValue;
		int BishopValue;
		int RookValue;
		int QueenValue;
		int BishopPairBonus;
		int PawnPSQT[64];
		int KnightPSQT[64];
		int BishopPSQT[64];
		int RookPSQT[64];
		int QueenPSQT[64];
		int KingEarlyPSQT[64];
		int KingLatePSQT[64];
	};

	Tuning(const std::string dataset);
	const float ConvertResult(const std::string str);
	const int Evaluate(Board &b);
	const double Sigmoid(const int score, const double K);
	const double FindBestK(std::vector<Board>& boards, std::vector<float>& results);
	const double CalculateMSE(const double K, std::vector<Board> &boards, std::vector<float> &results);
	const void UpdateWeightById(const int id, const int value);
	const int GetWeightById(const int id);
	const void Tune(double K);

	std::vector<float> TrainResults;
	std::vector<Board> TrainBoards;
	std::vector<float> TestResults;
	std::vector<Board> TestBoards;
	TempWeights Temp;


};

