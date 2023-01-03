#include "Tuning.h"

Tuning::Tuning(const std::string dataset) {
	cout << "\nStarting tuning..." << endl;
	cout << "Note: may take a looong time" << endl;

	// Initialize temporary weights
	
	Temp.PawnValue = Weights::PawnValue;
	Temp.KnightValue = Weights::KnightValue;
	Temp.BishopValue = Weights::BishopValue;
	Temp.RookValue = Weights::RookValue;
	Temp.QueenValue = Weights::QueenValue;
	Temp.BishopPairBonus = Weights::BishopPairBonus;
	for (int i = 0; i < 64; i++) Temp.PawnPSQT[i] = Weights::PawnPSQT[i];
	for (int i = 0; i < 64; i++) Temp.KnightPSQT[i] = Weights::KnightPSQT[i];
	for (int i = 0; i < 64; i++) Temp.BishopPSQT[i] = Weights::BishopPSQT[i];
	for (int i = 0; i < 64; i++) Temp.RookPSQT[i] = Weights::RookPSQT[i];
	for (int i = 0; i < 64; i++) Temp.QueenPSQT[i] = Weights::QueenPSQT[i];
	for (int i = 0; i < 64; i++) Temp.KingEarlyPSQT[i] = Weights::KingEarlyPSQT[i];
	for (int i = 0; i < 64; i++) Temp.KingLatePSQT[i] = Weights::KingLatePSQT[i];

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
	cout << "\nBest K found: K=" << K << endl;

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
	cout << "Test size:  " << TestBoards.size() << endl;

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
		totalError += pow((results[i] - Sigmoid(Evaluate(boards[i]), K)), 2);
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
		cout << ".";
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


		for (int i = 0; i <= 7 * 64 + 5; i++) {
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

			cout << "Iteration " << iterations << " param " << i << ": " << weightCurrent << " -> " << newWeight << " (" << weightCurrentMSE << " -> " << newMSE << ")" << endl;

			UpdateWeightById(i, newWeight);

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
			cout << "Stopping." << endl;
		}
		

		step -= 1;
		if (step == 0) step = 1;
		iterations += 1;
	}

	cout << "\n\n" << endl;

	for (int i = 0; i <= 64 * 7 + 5; i++) {
		cout << GetWeightById(i) << ", ";
		if (i % 64 == 63) cout << "\n";
	}

	cout << "\nCompleted." << endl;
}

const int Tuning::GetWeightById(const int id) {
	if (id < 64 * 1) { return Temp.PawnPSQT[id % 64]; }
	if (id < 64 * 2) { return Temp.KnightPSQT[id % 64]; }
	if (id < 64 * 3) { return Temp.BishopPSQT[id % 64]; }
	if (id < 64 * 4) { return Temp.RookPSQT[id % 64]; }
	if (id < 64 * 5) { return Temp.QueenPSQT[id % 64]; }
	if (id < 64 * 6) { return Temp.KingEarlyPSQT[id % 64]; }
	if (id < 64 * 7) { return Temp.KingLatePSQT[id % 64]; }

	if (id == 64 * 7) { return Temp.PawnValue; }
	if (id == 64 * 7 + 1) { return Temp.KnightValue; }
	if (id == 64 * 7 + 2) { return Temp.BishopValue; }
	if (id == 64 * 7 + 3) { return Temp.RookValue; }
	if (id == 64 * 7 + 4) { return Temp.QueenValue; }
	if (id == 64 * 7 + 5) { return Temp.BishopPairBonus; }
}

const void Tuning::UpdateWeightById(const int id, const int value) {
	if (id < 64 * 1) { Temp.PawnPSQT[id % 64] = value; return; }
	if (id < 64 * 2) { Temp.KnightPSQT[id % 64] = value; return; }
	if (id < 64 * 3) { Temp.BishopPSQT[id % 64] = value; return; }
	if (id < 64 * 4) { Temp.RookPSQT[id % 64] = value; return; }
	if (id < 64 * 5) { Temp.QueenPSQT[id % 64] = value; return; }
	if (id < 64 * 6) { Temp.KingEarlyPSQT[id % 64] = value; return; }
	if (id < 64 * 7) { Temp.KingLatePSQT[id % 64] = value; return; }

	if (id == 64 * 7) { Temp.PawnValue = value; return; }
	if (id == 64 * 7 + 1) { Temp.KnightValue = value; return; }
	if (id == 64 * 7 + 2) { Temp.BishopValue = value; return; }
	if (id == 64 * 7 + 3) { Temp.RookValue = value; return; }
	if (id == 64 * 7 + 4) { Temp.QueenValue = value; return; }
	if (id == 64 * 7 + 5) { Temp.BishopPairBonus = value; return; }
}


// Evaluation function cloned from search
// There's surely a way of merging these two together without sacrificing performance
const int Tuning::Evaluate(Board &board) {

	if (board.State == GameState::Draw) return 0;
	if (board.State == GameState::WhiteVictory) {
		if (board.Turn == Turn::White) return MateEval;
		if (board.Turn == Turn::Black) return -MateEval;
	}
	else if (board.State == GameState::BlackVictory) {
		if (board.Turn == Turn::White) return -MateEval;
		if (board.Turn == Turn::Black) return MateEval;
	}

	// 2. Materials
	int score = 0;
	score += Popcount(board.WhitePawnBits) * Temp.PawnValue;
	score += Popcount(board.WhiteKnightBits) * Temp.KnightValue;
	score += Popcount(board.WhiteBishopBits) * Temp.BishopValue;
	score += Popcount(board.WhiteRookBits) * Temp.RookValue;
	score += Popcount(board.WhiteQueenBits) * Temp.QueenValue;
	score -= Popcount(board.BlackPawnBits) * Temp.PawnValue;
	score -= Popcount(board.BlackKnightBits) * Temp.KnightValue;
	score -= Popcount(board.BlackBishopBits) * Temp.BishopValue;
	score -= Popcount(board.BlackRookBits) * Temp.RookValue;
	score -= Popcount(board.BlackQueenBits) * Temp.QueenValue;

	if (Popcount(board.WhiteBishopBits) >= 2) score += Temp.BishopPairBonus;
	if (Popcount(board.BlackBishopBits) >= 2) score -= Temp.BishopPairBonus;

	uint64_t occupancy = board.GetOccupancy();
	float phase = CalculateGamePhase(board);
	while (occupancy != 0) {
		uint64_t i = 64 - __lzcnt64(occupancy) - 1;
		SetBitFalse(occupancy, i);
		int piece = board.GetPieceAt(i);
		if (piece == Piece::WhitePawn) score += Temp.PawnPSQT[i];
		else if (piece == Piece::WhiteKnight) score += Temp.KnightPSQT[i];
		else if (piece == Piece::WhiteBishop) score += Temp.BishopPSQT[i];
		else if (piece == Piece::WhiteRook) score += Temp.RookPSQT[i];
		else if (piece == Piece::WhiteQueen) score += Temp.QueenPSQT[i];
		else if (piece == Piece::WhiteKing) score += GetTaperedPSQT(Temp.KingEarlyPSQT[i], Temp.KingLatePSQT[i], phase);

		if (piece == Piece::BlackPawn) score -= Temp.PawnPSQT[63 - i];
		else if (piece == Piece::BlackKnight) score -= Temp.KnightPSQT[63 - i];
		else if (piece == Piece::BlackBishop) score -= Temp.BishopPSQT[63 - i];
		else if (piece == Piece::BlackRook) score -= Temp.RookPSQT[63 - i];
		else if (piece == Piece::BlackQueen) score -= Temp.QueenPSQT[63 - i];
		else if (piece == Piece::BlackKing) score -= GetTaperedPSQT(Temp.KingEarlyPSQT[63 - i], Temp.KingLatePSQT[63 - i], phase);
	}

	return score;
}