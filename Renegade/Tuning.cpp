#include "Tuning.h"

Tuning::Tuning() {
	cout << "\nThis is the tuner for Renegade, based on Texel's method." << endl;
	cout << "Positions are loaded from positions.txt" << endl;
	cout << "How many positions to load (-1 for all of them)? ";
	int positionsToLoad;
	cin >> positionsToLoad;
	cout << "\nStarting tuning..." << endl;
	cout << "Note: may take a looong time" << endl;

	// Initialize temporary weights
	for (int i = 0; i < WeightsSize; i++) TempWeights[i] = Weights[i];

	// Load into memory
	std::ifstream ifs("positions.txt");
	std::string line;
	int lines = 0;
	std::vector<Board> loadedBoards;
	std::vector<float> loadedResults;
	while (std::getline(ifs, line) && ((lines < positionsToLoad) || (positionsToLoad == -1))) {
		lines += 1;
		std::string fen = line.substr(0, line.size() - 6);
		std::string result = line.substr(line.size() - 5);
		loadedResults.push_back(ConvertResult(result));
		loadedBoards.push_back(Board(fen));
	}
	cout << "Number of positions loaded: " << loadedBoards.size() << endl;

	// K parameter selection
	cout << "Finding best K..." << endl;
	double K = FindBestK(loadedBoards, loadedResults);
	cout << "Best K found: K=" << K << endl;

	// Splitting into train and test datasets
	const double trainRatio = 0.85;
	for (int i = 0; i < trainRatio * lines; i++) {
		TrainBoards.push_back(loadedBoards[i]);
		TrainResults.push_back(loadedResults[i]);
	}
	for (int i = static_cast<int>(trainRatio * lines); i < lines; i++) {
		TestBoards.push_back(loadedBoards[i]);
		TestResults.push_back(loadedResults[i]);
	}
	cout << "Train size: " << TrainBoards.size() << endl;
	cout << "Test size:  " << TestBoards.size() << '\n' << endl;

	// Start tuning
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
	return 1.0 / (1.0 + pow(10.0, -K * score / 400.0));
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
	return 0.7;
	double K = 0.50;
	const double maxK = 0.71;
	const double step = 0.01;

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
	int iterations = 1;
	double testMSE = CalculateMSE(K, TestBoards, TestResults);

	std::cout << std::fixed;
	std::cout << std::setprecision(6);

	// Change these to tune a specific weight
	int step = 2;
	std::vector<int> weightsForTuning;
	//for (int i = 0; i < WeightsSize; i++) weightsForTuning.push_back(i);
	weightsForTuning.push_back(IndexKingSafety(10));

	// Main optimizer loop
	// To do: use an efficient (e.g. adam) optimizer
	while (true) {
		int paramId = 0;
		improvements = 0;


		for (const int i: weightsForTuning) {
			cout << "Iteration " << iterations << ", tuning parameter " << i + 1 << " of " << WeightsSize << "...      " << '\r' << std::flush;
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

		for (int j = 0; j < 50; j++) cout << " ";
		cout << "\n\n\nWeights:" << endl;

		for (const int i: weightsForTuning) {
			cout << GetWeightById(i) << ", ";
			if (i % 64 == 63) cout << "\n";
		}

		cout << "\nChanges made during iteration " << iterations << ": " << improvements << endl;
		if (improvements == 0) break;

		double newTestMSE = CalculateMSE(K, TestBoards, TestResults);
		if (newTestMSE < testMSE) {
			cout << "Improved test MSE: " << testMSE << " -> " << newTestMSE << '\n' << endl;
			testMSE = newTestMSE;
		}
		else {
			cout << "Worsened test MSE: " << testMSE << " -> " << newTestMSE << '\n' << endl;
			if (step > 1) step /= 2;
			//else break;
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
