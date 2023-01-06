#include "Tuning.h"

Tuning::Tuning(const std::string dataset) {
	cout << "\nStarting tuning..." << endl;
	cout << "Note: may take a looong time" << endl;

	// Initialize temporary weights
	for (int i = 0; i < WeightsSize; i++) TempWeights[i] = Weights[i];

	std::ifstream ifs("positions.txt");
	std::string line;
	int lines = 0;
	std::vector<Board> loadedBoards;
	std::vector<float> loadedResults;
	while (std::getline(ifs, line)) {
		lines += 1;
		std::string fen = line.substr(0, line.size() - 6);
		std::string result = line.substr(line.size() - 5);
		loadedResults.push_back(ConvertResult(result));
		loadedBoards.push_back(Board(fen));
	}
	cout << "Number of positions loaded: " << loadedBoards.size() << endl;
	cout << "Finding best K..." << endl;
	double K = FindBestK(loadedBoards, loadedResults);
	cout << "Best K found: K=" << K << endl;

	const float trainRatio = 0.85;
	for (int i = 0; i < trainRatio * lines; i++) {
		TrainBoards.push_back(loadedBoards[i]);
		TrainResults.push_back(loadedResults[i]);
	}
	for (int i = trainRatio * lines; i < lines; i++) {
		TestBoards.push_back(loadedBoards[i]);
		TestResults.push_back(loadedResults[i]);
	}
	cout << "Train size: " << TrainBoards.size() << endl;
	cout << "Test size:  " << TestBoards.size() << '\n' << endl;

	Tune(K);

}

const float Tuning::ConvertResult(const std::string str) {
	if (str == "[1.0]") return 1; // White win
	if (str == "[0.5]") return 0.5; // Draw
	if (str == "[0.0]") return 0; // Black win
	cout << "!!! Invalid result: '" << str << "' !!!" << endl;
	return 0.5;
}


const double Tuning::Sigmoid(const int score, const double K) {
	return 1 / (1 + pow(10, -K * (double)score / 400.0));
}

const double Tuning::CalculateMSE(const double K, std::vector<Board>& boards, std::vector<float>& results) {
	double totalError = 0;
	for (int i = 0; i < boards.size(); i++) {
		const int sign = boards[i].Turn == Turn::White ? 1 : -1; // Whoops, this is *kinda* needed
		totalError += pow((results[i] - Sigmoid(sign * EvaluateBoard(boards[i], 0, TempWeights), K)), 2);
	}
	return totalError / boards.size();
}

const double Tuning::FindBestK(std::vector<Board>& boards, std::vector<float>& results) {
	double K = 0.65;
	const double maxK = 0.75;
	const double step = 0.001;

	double bestK = 0;
	double bestError = 1;

	while (K <= maxK) {
		double Kerror = CalculateMSE(K, boards, results);
		if (Kerror < bestError) {
			bestError = Kerror;
			bestK = K;
		}
		K += step;
	}
	return bestK;
}

const void Tuning::Tune(const double K) {
	int improvements = 0;
	int step = 10;
	int iterations = 1;
	double testMSE = CalculateMSE(K, TestBoards, TestResults);

	std::cout << std::fixed;
	std::cout << std::setprecision(6);

	while (true) {
		int paramId = 0;
		improvements = 0;


		for (int i = 0; i < WeightsSize; i++) {
			int weightCurrent = GetWeightById(i);
			int weightPlus = weightCurrent + step;
			int weightMinus = weightCurrent - step;


			double weightCurrentMSE = CalculateMSE(K, TrainBoards, TrainResults);
			UpdateWeightById(i, weightMinus);
			double weightMinusMSE = CalculateMSE(K, TrainBoards, TrainResults);
			UpdateWeightById(i, weightPlus);
			double weightPlusMSE = CalculateMSE(K, TrainBoards, TrainResults);

			int newWeight = 999;
			double newMSE = 999;
			if ((weightMinusMSE < weightCurrentMSE) && (weightMinusMSE < weightPlusMSE)) {
				newWeight = weightMinus;
				newMSE = weightMinusMSE;
				improvements += 1;
			}
			else if ((weightPlusMSE < weightCurrentMSE) && (weightPlusMSE < weightMinusMSE)) {
				newWeight = weightPlus;
				newMSE = weightPlusMSE;
				improvements += 1;
			}
			else {
				newWeight = weightCurrent;
				newMSE = weightCurrentMSE;
			}

			//cout << "Iteration " << iterations << " param " << i << ": " << weightCurrent << " -> " << newWeight << " (" << weightCurrentMSE << " -> " << newMSE << ")" << endl;

			UpdateWeightById(i, newWeight);

		}

		cout << "\n\nWeights:" << endl;
		for (int i = 0; i <= WeightsSize; i++) {
			cout << GetWeightById(i) << ", ";
			if (i % 64 == 63) cout << "\n";
		}

		cout << "\nChanges made this iteration: " << improvements << endl;
		if (improvements == 0) break;

		double newTestMSE = CalculateMSE(K, TestBoards, TestResults);
		if (newTestMSE < testMSE) {
			cout << "Improved test MSE: " << testMSE << " -> " << newTestMSE << '\n' << endl;
			testMSE = newTestMSE;
		}
		else {
			cout << "Worsened test MSE: " << testMSE << " -> " << newTestMSE << '\n' << endl;
			if (step > 1) step /= 2;
			else break;
		}
		

		step -= 1;
		if (step == 0) step = 1;
		iterations += 1;
	}
	cout << "Stopping." << endl;
	cout << "\nCompleted." << endl;
}

const int Tuning::GetWeightById(const int id) {
	return TempWeights[id];
}

const void Tuning::UpdateWeightById(const int id, const int value) {
	TempWeights[id] = value;
}