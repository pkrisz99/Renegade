#include "Datagen.h"

Datagen::Datagen() {
	//
}

void Datagen::Start() {

	const int startingEvalLimit = 500;
	const int softNodeLimit = 5000;
	const int hardNodeLimit = 15000;
	const int depthLimit = 20;
	const int verificationDepth = 10;
	const int randomPlyBase = 14;

	cout << "\nRenegade's data generation utility\n" << endl;

	std::string filename;
	cout << "Filename? ";
	cin >> filename;
	cout << endl;

	int threadCount;
	cout << "How many threads? ";
	cin >> threadCount;
	cout << endl;

	cout << "Datagen settings: " << endl;
	cout << " - " << randomPlyBase << " or " << randomPlyBase + 1 << " random plies, then rollout" << endl;
	cout << " - Verification at depth " << verificationDepth << " with a threshold of " << startingEvalLimit << endl;
	cout << " - Playing with a soft node limit of " << softNodeLimit << '\n' << endl;
	//cout << " - Adjudication if mate is reported for 2 plies, or if eval<20  for 20 plies\n" << endl;


	EngineSettings settings = EngineSettings();
	SearchParams params = SearchParams();
	params.softnodes = softNodeLimit;
	params.nodes = hardNodeLimit;
	params.depth = depthLimit;
	SearchParams verificationParams = SearchParams();
	verificationParams.depth = verificationDepth;

	StartTime = Clock::now();

	std::vector<std::thread> threads = std::vector<std::thread>();
	for (int i = 1; i <= threadCount; i++) {
		std::string filenameForThread = filename + "_" + std::to_string(i);
		threads.emplace_back(&Datagen::SelfPlay, this, filenameForThread, std::ref(params), std::ref(verificationParams),
			std::ref(settings), randomPlyBase, startingEvalLimit, i - 1);
	}
	for (std::thread& t : threads) t.join();


}

void Datagen::SelfPlay(const std::string filename, const SearchParams params, const SearchParams vParams, const EngineSettings settings,
	const int randomPlyBase, const int startingEvalLimit, const int threadId) {

	Search* Searcher1 = new Search;
	Search* Searcher2 = new Search;

	Searcher1->Heuristics.SetHashSize(1);
	Searcher2->Heuristics.SetHashSize(1);
	
	Results results;
	int gamesOnThread = 0;
	std::mt19937 generator(std::random_device{}());

	std::vector<std::pair<std::string, int>> CurrentFENs;
	std::vector<std::string> unsavedFENs;

	while (true) {

		// 1. Reset state
		Board board = Board(FEN::StartPos);
		Searcher1->ResetState(true);
		Searcher2->ResetState(true);
		Searcher1->DatagenMode = true;
		Searcher2->DatagenMode = true;
		bool failed = false;

		// 2. Generate random moves from the start
		int randomPlies = (Games % 2 == 0) ? randomPlyBase : (randomPlyBase + 1);
		for (int i = 0; i < randomPlies; i++) {
			std::vector<Move> moves = std::vector<Move>();
			board.GenerateMoves(moves, MoveGen::All, Legality::Legal);
			if (moves.size() == 0) {
				failed = true;
				break;
			}
			std::uniform_int_distribution<std::size_t> distribution(0, moves.size() - 1);
			board.Push(moves[distribution(generator)]);
		}
		if (failed) continue;

		// 3. Verify evaluation if acceptable
		results = Searcher1->SearchMoves(board, vParams, settings, false);
		if (std::abs(results.score) > startingEvalLimit) failed = true;
		if (failed) continue;
		Searcher1->ResetState(true);

		CurrentFENs.clear();
		GameState outcome = GameState();
		int adjudicationCounter = 0;

		// 4. Play out the game
		while (true) {
			// Search
			Search* currentSearcher = (board.Turn == Turn::White) ? Searcher1 : Searcher2; // ???????
			results = currentSearcher->SearchMoves(board, params, settings, false);
			int whiteScore = results.score * (board.Turn == Turn::Black ? -1 : 1);

			// Adjudicate
			if (std::abs(whiteScore) > MateThreshold) {
				adjudicationCounter += 1;
				if (adjudicationCounter >= 2) {
					if (whiteScore > 0) outcome = GameState::WhiteVictory;
					else outcome = GameState::BlackVictory;
				}
			}
			else adjudicationCounter = 0;

			if (board.HalfmoveClock > 85) {
				outcome = GameState::Draw;
				//cout << "Draw adjud" << endl;
				break;
			}
			if (board.GetPlys() > 600) {
				failed = true;
				break;
			}

			// Check position if it should be stored
			PositionsTotal += 1;
			if (!Filter(board, results.BestMove(), results.score)) {
				PositionsAccepted += 1;
				CurrentFENs.push_back(std::pair(board.GetFEN(), whiteScore));
			}
			board.Push(results.BestMove());


			outcome = board.GetGameState();
			if (outcome != GameState::Playing) break;

		}

		if (failed) continue;

		Games += 1;
		gamesOnThread += 1;

		// 5. Store the game
		for (const auto& position : CurrentFENs) {
			const std::string marlinformat = ToMarlinformat(position, outcome);
			unsavedFENs.push_back(marlinformat);
		}
		if (gamesOnThread % 16 == 0) { // periodically save file
			std::ofstream file(filename, std::ios_base::app);
			for (const auto& line : unsavedFENs) file << line << '\n';
			file.close();
			unsavedFENs.clear();
		}

		// 6. Update display
		if (Games % 10 == 0) {
			const auto endTime = Clock::now();
			const int seconds = static_cast<int>((endTime - StartTime).count() / 1e9);
			const int speed = PositionsAccepted * 3600 / std::max(seconds, 1);

			std::string display = "";
			display = "Games: " + std::to_string(Games) + "  /   Positions accepted: " + std::to_string(PositionsAccepted) + "  /   Runtime: "
				+ std::to_string(seconds) + "s  /  " + std::to_string(speed) + " per hour   ";
			cout << display << '\r';
		}


	}
}


bool Datagen::Filter(const Board& board, const Move& move, const int eval) const {
	if (std::abs(eval) > MateThreshold) return true;
	if (!board.IsMoveQuiet(move)) return true;
	if (board.IsInCheck()) return true;
	return false;
}

std::string Datagen::ToMarlinformat(const std::pair<std::string, int>& position, const GameState outcome) const {
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

void Datagen::ShuffleEntries() {
	cout << "\nRenegade's data shuffling utility for datagen\n";

	// Ask for filename
	cout << "Filename? ";
	std::string filename;
	cin >> filename;
	cout << endl;
	
	// Read the file
	std::ifstream ifs(filename);
	std::vector<std::string> lines;
	std::string line;
	lines.reserve(50'000'000);

	while (std::getline(ifs, line)) lines.push_back(line);
	cout << "Entry count: " << lines.size() << endl;
	//ifs.close();

	// Shuffle entries
	auto rng = std::default_random_engine();
	std::shuffle(std::begin(lines), std::end(lines), rng);

	// Write file
	std::ofstream output(filename + "_shuffled");
	std::ostream_iterator<std::string> outputIterator(output, "\n");
	std::copy(lines.begin(), lines.end(), outputIterator);

	cout << "Complete.\n" << endl;
	
}

void Datagen::MergeFiles() {
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

	
	std::vector<std::string> lines;
	lines.reserve(50'000'000);
	const std::string mergedName = name + "_merged";

	for (const auto& filename : found) {
		lines.clear();

		// Read files
		std::ifstream ifs(filename);
		std::string line;
		while (std::getline(ifs, line)) lines.push_back(line);
		cout << filename << " -> entry count: " << lines.size() << endl;
		
		// Write file
		std::ofstream output;
		output.open(mergedName, std::ios_base::app);
		for (const auto& line : lines) output << line << endl;
		output.close();
	}

	cout << "\nComplete.\n" << endl;
	
}