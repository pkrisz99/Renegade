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
	for (int i = 0; i < TempWeights.WeightSize; i++) TempWeights.Weights[i] = Weights.Weights[i];

	// Load into memory
	std::ifstream ifs("positions.txt");
	std::string line;
	int lines = 0;
	std::vector<Board> loadedBoards;
	std::vector<float> loadedResults;
	while (std::getline(ifs, line) && ((lines < positionsToLoad) || (positionsToLoad == -1))) {
		lines += 1;
		std::vector<std::string> parts = Split(line);
		std::string fen = parts[0] + " " + parts[1] + " " + parts[2] + " " + parts[3] + " " + parts[4] + " " + parts[5];
		loadedBoards.push_back(Board(fen));
		loadedResults.push_back(ConvertResult(parts[6]));
	}
	cout << "Number of positions loaded: " << loadedBoards.size() << endl;

	// K parameter selection
	cout << "Finding best K..." << endl;
	double K = FindBestK(loadedBoards, loadedResults);
	cout << "Best K found: K=" << K << endl;

	// Splitting into train and test datasets
	const double trainRatio = 0.8;
	std::random_device dev;
	std::mt19937 rng(0); // rng(dev());
	std::uniform_real_distribution random(0.f, 1.f);
	for (int i = 0; i < lines; i++) {
		const float r = random(rng);
		if (r <= trainRatio) {
			TrainBoards.push_back(loadedBoards[i]);
			TrainResults.push_back(loadedResults[i]);
		}
		else {
			TestBoards.push_back(loadedBoards[i]);
			TestResults.push_back(loadedResults[i]);
		}
	}
	cout << "Train size: " << TrainBoards.size() << endl;
	cout << "Test size:  " << TestBoards.size() << '\n' << endl;

	cout << "Initial MSE: " << CalculateMSE(K, TestBoards, TestResults) << endl;

	// Start tuning
	Tune(K);

}

float Tuning::ConvertResult(const std::string str) {
	if (str == "[1.0]") return 1;
	if (str == "[0.5]") return 0.5;
	if (str == "[0.0]") return 0;
	cout << "!!! Invalid result: '" << str << "' !!!" << endl;
	return 0.5;
}


double Tuning::Sigmoid(const int score, const double K) {
	return 1.0 / (1.0 + pow(10.0, -K * score / 400.0));
}

double Tuning::CalculateMSE(const double K, std::vector<Board>& boards, std::vector<float>& results) {
	double totalError = 0;
	for (int i = 0; i < boards.size(); i++) {
		const int sign = boards[i].Turn == Turn::White ? 1 : -1;
		totalError += pow((results[i] - Sigmoid(sign * EvaluateBoard(boards[i], TempWeights), K)), 2);
	}
	return totalError / boards.size();
}

double Tuning::FindBestK(std::vector<Board>& boards, std::vector<float>& results) {
	return 1.30;

	double K = 1.2;
	const double maxK = 1.45;
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
	cout << bestError << endl;
	return bestK;
}

void Tuning::Tune(const double K) {
	int improvements = 0;
	int iterations = 1;
	double testMSE = CalculateMSE(K, TestBoards, TestResults);

	std::cout << std::fixed;
	std::cout << std::setprecision(6);

	// Change these to tune a specific weight
	std::vector<ParamSettings> weightsForTuning;
	const int defaultStep = 3;
	
	// Tune material values
	//for (int i = 1; i <= 5; i++) weightsForTuning.push_back({ TempWeights.IndexPieceMaterial(i), defaultStep, defaultStep, true, true });
	//weightsForTuning.push_back({ TempWeights.IndexBishopPair, defaultStep, defaultStep, true, true });
	
	// Pawn values
	/*
	for (int i = 0; i <= 63; i++) weightsForTuning.push_back({ TempWeights.IndexPSQT(PieceType::Pawn, i), defaultStep, defaultStep, true, true });
	for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexPassedPawn(i), defaultStep, defaultStep, true, true });
	for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexPawnPhalanx(i), defaultStep, defaultStep, true, true });
	for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexPawnSupportingPawn(i), defaultStep, defaultStep, true, true });
	for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexBlockedPasser(i), defaultStep, defaultStep, true, true });
	for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexIsolatedPawn(i), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexDoubledPawns, defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexTripledPawns, defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexPieceMaterial(PieceType::Pawn), defaultStep, defaultStep, true, true }); */

	// Knight values
	/*for (int i = 0; i <= 63; i++) weightsForTuning.push_back({TempWeights.IndexPSQT(PieceType::Knight, i), defaultStep, defaultStep, true, true});
	for (int i = 0; i <= 8; i++) weightsForTuning.push_back({ TempWeights.IndexKnightMobility(i), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexPieceMaterial(PieceType::Knight), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexKnightOutpost, defaultStep, defaultStep, true, true });*/

	// Bishop values
	/*for (int i = 0; i <= 63; i++) weightsForTuning.push_back({TempWeights.IndexPSQT(PieceType::Bishop, i), defaultStep, defaultStep, true, true});
	for (int i = 0; i <= 13; i++) weightsForTuning.push_back({ TempWeights.IndexBishopMobility(i), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexPieceMaterial(PieceType::Bishop), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexBishopPair, defaultStep, defaultStep, true, true });*/

	// Rook values
	/*for (int i = 0; i <= 63; i++) weightsForTuning.push_back({TempWeights.IndexPSQT(PieceType::Rook, i), defaultStep, defaultStep, true, true});
	for (int i = 0; i <= 14; i++) weightsForTuning.push_back({ TempWeights.IndexRookMobility(i), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexPieceMaterial(PieceType::Rook), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexRookOnOpenFile, defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexRookOnSemiOpenFile, defaultStep, defaultStep, true, true });*/

	// Queen values
	/*for (int i = 0; i <= 63; i++) weightsForTuning.push_back({TempWeights.IndexPSQT(PieceType::Queen, i), defaultStep, defaultStep, true, true});
	for (int i = 0; i <= 27; i++) weightsForTuning.push_back({ TempWeights.IndexQueenMobility(i), defaultStep, defaultStep, true, true });
	weightsForTuning.push_back({ TempWeights.IndexPieceMaterial(PieceType::Queen), defaultStep, defaultStep, true, true });*/

	// King values
	//for (int i = 0; i <= 63; i++) weightsForTuning.push_back({ TempWeights.IndexPSQT(PieceType::King, i), defaultStep, defaultStep, true, true });

	// Threats
	/*for (int i = 1; i <= 5; i++) weightsForTuning.push_back({TempWeights.IndexPawnThreats(i), defaultStep, defaultStep, true, true});
	for (int i = 1; i <= 5; i++) weightsForTuning.push_back({ TempWeights.IndexKnightThreats(i), defaultStep, defaultStep, true, true });
	for (int i = 1; i <= 5; i++) weightsForTuning.push_back({ TempWeights.IndexBishopThreats(i), defaultStep, defaultStep, true, true });
	for (int i = 1; i <= 5; i++) weightsForTuning.push_back({ TempWeights.IndexRookThreats(i), defaultStep, defaultStep, true, true });
	for (int i = 1; i <= 5; i++) weightsForTuning.push_back({ TempWeights.IndexQueenThreats(i), defaultStep, defaultStep, true, true });
	for (int i = 1; i <= 5; i++) weightsForTuning.push_back({ TempWeights.IndexKingThreats(i), defaultStep, defaultStep, true, true });*/

	// King safety
	for (int i = 1; i <= 25; i++) weightsForTuning.push_back({ TempWeights.IndexKingDanger(i), defaultStep, defaultStep, true, true });

	//for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexPassedPawn(i), defaultStep, defaultStep, true, true });
	//for (int i = 0; i <= 7; i++) weightsForTuning.push_back({ TempWeights.IndexBlockedPasser(i), defaultStep, defaultStep, true, true });
	//weightsForTuning.push_back({ TempWeights.IndexDoubledPawns, defaultStep, defaultStep, true, true });
	//weightsForTuning.push_back({ TempWeights.IndexTripledPawns, defaultStep, defaultStep, true, true });

	// Main optimizer loop (to do: use an efficient e.g. adam optimizer)
	while (true) {
		improvements = 0;
		auto startTime = Clock::now();

		// Iterated through the tuned weights
		int doneInIteration = 0;
		int triedInIteration = 0;
		for (ParamSettings& p: weightsForTuning) {

			// Iterate through early and lategame values
			for (const bool phase : {early, late}) {

				if ((phase == early) && !p.tuneEarly) continue;
				if ((phase == late) && !p.tuneLate) continue;
				triedInIteration += 1;

				const int iterationPercent = static_cast<int>(doneInIteration * 100 / weightsForTuning.size());

				cout << "Iteration " << iterations << ", tuning parameter " << p.id << ((phase == early) ? " (early)" : " (late) ")
					<< " of " << TempWeights.WeightSize << " (" << iterationPercent << "%)...      " << '\r' << std::flush;

				const int weightCurrent = GetWeightById(p.id, phase);
				const int weightPlus = weightCurrent + ((phase == early) ? p.earlyStep : p.lateStep);
				const int weightMinus = weightCurrent - ((phase == early) ? p.earlyStep : p.lateStep);

				const double weightCurrentMSE = CalculateMSE(K, TrainBoards, TrainResults);
				UpdateWeightById(p.id, phase, weightMinus);
				const double weightMinusMSE = CalculateMSE(K, TrainBoards, TrainResults);
				UpdateWeightById(p.id, phase, weightPlus);
				const double weightPlusMSE = CalculateMSE(K, TrainBoards, TrainResults);

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
					if ((phase == early) && (p.earlyStep > 1)) p.earlyStep = std::max(1, p.earlyStep / 2);
					if ((phase == late) && (p.lateStep > 1)) p.lateStep = std::max(1, p.lateStep / 2);
				}

				UpdateWeightById(p.id, phase, newWeight);
				//cout << improvements << endl;
			}

			doneInIteration += 1;

		}

		for (int j = 0; j < 70; j++) cout << " ";
		cout << "\n\n\nWeights:" << endl;

		int j = 0;
		for (const ParamSettings& p: weightsForTuning) {
			j += 1;
			cout << TempWeights.Weights[p.id] << ", ";
			if ((j != 0) && (j % 64 == 0)) cout << "\n";
		}
		double newTestMSE = CalculateMSE(K, TestBoards, TestResults);

		cout << "\nChanges made during iteration " << iterations << ": " << improvements << " (out of " << triedInIteration << ")" << endl;
		auto endTime = Clock::now();
		int seconds = static_cast<int>((endTime - startTime).count() / 1e9);
		cout << "Iteration took " << seconds << " seconds" << endl;

		if (newTestMSE < testMSE) {
			cout << "Improved test MSE: " << testMSE << " -> " << newTestMSE << " (difference: " << int((testMSE - newTestMSE) * 1e6) << ")" << '\n' << endl;
			testMSE = newTestMSE;
		}
		else {
			cout << "Worsened test MSE: " << testMSE << " -> " << newTestMSE << '\n' << endl;
			break;
		}

		if (improvements == 0) break;
		iterations += 1;
	}
	cout << "Stopping." << endl;
	cout << "\nCompleted." << endl;
}

int Tuning::GetWeightById(const int id, const bool isEarlygame) {
	if (isEarlygame) return TempWeights.Weights[id].early;
	else return TempWeights.Weights[id].late;
}

void Tuning::UpdateWeightById(const int id, const bool isEarlygame, const int value) {
	if (isEarlygame) TempWeights.Weights[id].early = value;
	else  TempWeights.Weights[id].late = value;
}
