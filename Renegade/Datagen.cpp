#include "Datagen.h"

Datagen::Datagen() {
	//
}

void Datagen::Start() {

	Console::ClearScreen();
	cout << Console::Highlight << " Renegade's datagen tool " << Console::White << "\n" << endl;

	std::string filename;
	cout << "Filename? " << Console::Yellow;
	cin >> filename;

	cout << Console::White << "How many threads? " << Console::Yellow;
	cin >> ThreadCount;

	cout << Console::White << "Do DRFC? " << Console::Yellow;
	cin >> DFRC;
	cout << Console::White << endl;
	Settings::Chess960 = DFRC;

	cout << "Datagen settings: " << endl;
	cout << " - " << Console::Yellow << randomPlyBase << Console::White << " or " << Console::Yellow << randomPlyBase + 1
		<< Console::White << " plies of random rollout, then normal playout" << endl;
	cout << " - Verification at depth " << Console::Yellow << verificationDepth << Console::White
		<< " with a threshold of " << Console::Yellow << startingEvalLimit << Console::White << endl;
	cout << " - Playing with a soft node limit of " << Console::Yellow << softNodeLimit << Console::White << endl;
	cout << " - Adjudication if mate is reported for 2 plies" << endl;
	cout << " - Using DFRC starting positions: " << Console::Yellow << std::boolalpha << DFRC
		<< std::noboolalpha << Console::White << "\n" << endl;


	SearchParams params = SearchParams();
	params.softnodes = softNodeLimit;
	params.nodes = hardNodeLimit;
	params.depth = depthLimit;
	SearchParams verificationParams = SearchParams();
	verificationParams.depth = verificationDepth;
	Settings::Hash = 1;

	StartTime = Clock::now();

	std::vector<std::thread> threads = std::vector<std::thread>();
	for (int i = 1; i <= ThreadCount; i++) {
		std::string filenameForThread = filename + "_" + std::to_string(i);
		threads.emplace_back(&Datagen::SelfPlay, this, filenameForThread, std::ref(params), std::ref(verificationParams), randomPlyBase, startingEvalLimit, i - 1);
	}
	for (std::thread& t : threads) t.join();

}

void Datagen::SelfPlay(const std::string filename, const SearchParams params, const SearchParams vParams,
	const int randomPlyBase, const int startingEvalLimit, const int threadId) {

	Search* Searcher1 = new Search;
	Search* Searcher2 = new Search;

	Searcher1->TranspositionTable.SetSize(Settings::Hash);
	Searcher2->TranspositionTable.SetSize(Settings::Hash);
	
	Results results;
	int gamesOnThread = 0;
	std::mt19937 generator(std::random_device{}());

	std::vector<std::pair<std::string, int>> CurrentFENs;
	std::vector<std::string> unsavedFENs;

	while (true) {

		// 1. Reset state
		Position position = [&] {
			if (!DFRC) return Position(FEN::StartPos);
			else {
				std::uniform_int_distribution<std::size_t> distribution(0, 959);
				const int whiteFrcIndex = distribution(generator);
				const int blackFrcIndex = distribution(generator);
				return Position(whiteFrcIndex, blackFrcIndex);
			}
		}();
		
		Searcher1->ResetState(true);
		Searcher2->ResetState(true);
		Searcher1->DatagenMode = true;
		Searcher2->DatagenMode = true;
		bool failed = false;

		// 2. Generate random moves from the start
		const int randomPlies = (Games % 2 == 0) ? randomPlyBase : (randomPlyBase + 1);
		for (int i = 0; i < randomPlies; i++) {
			MoveList moves{};
			position.GenerateMoves(moves, MoveGen::All, Legality::Legal);
			if (moves.size() == 0) {
				failed = true;
				break;
			}
			std::uniform_int_distribution<std::size_t> distribution(0, moves.size() - 1);
			position.Push(moves[distribution(generator)].move);
		}
		if (failed) continue;

		// 3. Verify evaluation if acceptable
		results = Searcher1->SearchMoves(position, vParams, false);
		if (std::abs(results.score) > startingEvalLimit) failed = true;
		if (failed) continue;
		Searcher1->ResetState(true);

		CurrentFENs.clear();
		GameState outcome = GameState();
		int adjudicationCounter = 0;

		// 4. Play out the game
		while (true) {
			// Search
			Search* currentSearcher = (position.Turn() == Side::White) ? Searcher1 : Searcher2;
			results = currentSearcher->SearchMoves(position, params, false);
			const int whiteScore = results.score * (position.Turn() == Side::Black ? -1 : 1);

			// Adjudicate
			if (std::abs(whiteScore) > MateThreshold) {
				adjudicationCounter += 1;
				if (adjudicationCounter >= 2) {
					if (whiteScore > 0) outcome = GameState::WhiteVictory;
					else outcome = GameState::BlackVictory;
					break;
				}
			}
			else adjudicationCounter = 0;

			if (position.GetPlys() > 600) {
				failed = true;
				//cout << "\nGame too long: " << board.GetFEN() << endl;
				break;
			}
			if (results.BestMove().IsNull()) {
				failed = true;
				cout << "\nGot null-move for " << position.GetFEN() << " with eval of " << results.score << endl;
				break;
			}

			// Check position if it should be stored
			PositionsTotal += 1;
			if (!Filter(position, results.BestMove(), results.score)) {
				PositionsAccepted += 1;
				CurrentFENs.push_back(std::pair(position.GetFEN(), whiteScore));
			}
			position.Push(results.BestMove());


			outcome = position.GetGameState();
			if (outcome != GameState::Playing) break;

		}

		if (failed) continue;

		Games += 1;
		gamesOnThread += 1;

		// 5. Store the game
		for (const auto& position : CurrentFENs) {
			const std::string marlinformat = ToTextformat(position, outcome);
			unsavedFENs.push_back(marlinformat);
		}
		if (gamesOnThread % 64 == 0) { // periodically save file
			std::ofstream file(filename, std::ios_base::app);
			for (const auto& line : unsavedFENs) file << line << '\n';
			file.close();
			unsavedFENs.clear();
		}

		// 6. Update display
		if (Games % 50 == 0) {
			const auto endTime = Clock::now();
			const int seconds = static_cast<int>((endTime - StartTime).count() / 1e9);
			const int speed1 = PositionsAccepted * 3600 / std::max(seconds, 1);
			const int speed2 = speed1 / 3600 / ThreadCount;

			std::string display = "";
			display = "Games: " + std::to_string(Games) + "  |  Positions accepted: " + std::to_string(PositionsAccepted) + "  |  Runtime: "
				+ std::to_string(seconds) + "s  |  " + std::to_string(speed1) + " per hour  (" + std::to_string(speed2) + "/s/th)";
			cout << display << '\r';
		}


	}
}


bool Datagen::Filter(const Position& pos, const Move& move, const int eval) const {
	if (std::abs(eval) > MateThreshold) return true;
	if (pos.GetPlys() < minSavePly) return true;
	if (DFRC && move.IsCastling()) return true;
	if (!pos.IsMoveQuiet(move)) return true;
	if (pos.IsInCheck()) return true;
	return false;
}

std::string Datagen::ToTextformat(const std::pair<std::string, int>& position, const GameState outcome) const {
	std::string outcomeStr;
	switch (outcome) {
	case GameState::WhiteVictory:
		outcomeStr = "1.0"; break;
	case GameState::BlackVictory:
		outcomeStr = "0.0"; break;
	case GameState::Draw:
		outcomeStr = "0.5"; break;
	default:
		cout << "????" << endl;
	}
	return position.first + " | " + std::to_string(position.second) + " | " + outcomeStr;
}


void Datagen::LowPlyFilter() const {
	cout << "\nFiltering low ply entries\n";

	// Ask for filename
	cout << "Filename? ";
	std::string filename;
	cin >> filename;
	cout << endl;

	cout << "Min ply? ";
	int minPly;
	cin >> minPly;
	cout << endl;

	std::string outputName = filename + "_min" + std::to_string(minPly);

	std::ofstream output;
	output.open(outputName, std::ios_base::app);

	// Read the file
	std::ifstream ifs(filename);
	std::string line;
	
	int counter = 0;

	while (std::getline(ifs, line)) {
		counter++;
		if (counter % 1'000'000 == 0) cout << counter << endl;

		const std::vector<std::string> parts = Split(line);
		const int ply = (stoi(parts[5]) - 1) * 2 + (parts[1] == "w" ? 0 : 1);

		if (ply < minPly) continue;
		output << line << endl;
	}
	output.close();

	cout << "Entry count: " << counter << endl;
	cout << "Complete.\n" << endl;
}

void Datagen::MergeFiles() const {
	cout << "\nRenegade's file merging utility for datagen\n";

	std::filesystem::path path = std::filesystem::current_path();
	cout << "Current folder: " << path << endl;

	std::string name;
	cout << "\nWhat is the base name of the generated files? ";
	cin >> name;

	std::vector<std::string> found;

	// Iterate through files in directory
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		auto filename = entry.path().filename();
		if (filename.string().starts_with(name)) {
			found.push_back(filename.string());
		}
	}
	cout << "Found " << found.size() << " files\n" << endl;


	const std::string mergedName = name + "_merged";

	// Write file
	std::ofstream output;
	output.open(mergedName, std::ios_base::app);
	int counter = 0;

	for (const auto& filename : found) {

		std::ifstream ifs(filename);
		std::string line;


		while (std::getline(ifs, line)) {
			counter += 1;
			if (counter % 1'000'000 == 0) cout << ".";
			output << line << endl;
		}
		ifs.close();

		cout << filename << " -> processed: " << counter << endl;
	}

	output.close();

	cout << "\nComplete.\n" << endl;

}