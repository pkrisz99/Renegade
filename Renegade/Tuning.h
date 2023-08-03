#pragma once
#include "Board.h"
#include "Evaluation.h"
#include "Utils.h"
#include <fstream>
#include <iomanip>
#include <random>

// This is the header file of Renegade's tuner. It is based on the Texel tuning method, and runs until no improvement can be found.
// It is almost like a single layer neural network. Uses separate training and test datasets.
// 
// Recommended to run this on at least a million positions, depending on the number of parameters to be tuned, it can take about a
// minute, or a day.
// Edit `Tuning.cpp` to customize the tuning behavior.

class Tuning
{
public:
	Tuning();
	float ConvertResult(const std::string str);
	double Sigmoid(const int score, const double K);
	double FindBestK(std::vector<Board>& boards, std::vector<float>& results);
	void UpdateWeightById(const int id, const bool isEarlygame, const int value);
	int GetWeightById(const int id, const bool isEarlygame);
	void Tune(double K);
	double CalculateMSE(const double K, std::vector<Board>& boards, std::vector<float>& results);
	//double MultithreadedCalculateMSE(const double K, std::vector<Board>& boards, std::vector<float>& results);
	//double PartialCalculateMSE(const double K, std::vector<Board>& boards, std::vector<float>& results, const int start, const int end);

	std::vector<float> TrainResults;
	std::vector<Board> TrainBoards;
	std::vector<float> TestResults;
	std::vector<Board> TestBoards;
	EvaluationFeatures TempWeights;

	const bool early = true;
	const bool late = false;

};

struct ParamSettings {
	int id;
	int earlyStep;
	int lateStep;
	bool tuneEarly;
	bool tuneLate;
};
