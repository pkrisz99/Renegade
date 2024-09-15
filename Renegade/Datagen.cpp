#include "Datagen.h"

void Datagen::Start(const bool quickStart) {

	// Quick start is to provide a streamlined way to launch datagen from the command line using the default settings
	// When launched from UCI, the settings has to be provided manually

	Console::ClearScreen();
	cout << Console::Highlight << " Renegade's datagen utility " << Console::White;
	cout << " [" << Version << "]\n" << endl;

	std::string filename;

	if (!quickStart) {
		cout << "Filename? " << Console::Yellow;
		cin >> filename;

		cout << Console::White << "How many threads? " << Console::Yellow;
		cin >> ThreadCount;

		cout << Console::White << "Do DRFC? " << Console::Yellow;
		cin >> DFRC;
		cout << Console::White << endl;
	}
	else {
		const auto time = std::chrono::system_clock::now().time_since_epoch();
		const uint64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(time).count();
		filename = "datagen_" + std::to_string(timestamp);
		ThreadCount = std::thread::hardware_concurrency();
		DFRC = false;
		
		cout << "Quick start details:" << endl;
		cout << " - Filename: " << Console::Yellow << filename << Console::White << endl;
		cout << " - Thread count: " << Console::Yellow << ThreadCount << Console::White << endl;
		cout << " - Doing DFRC: " << Console::Yellow << DFRC << Console::White << '\n' << endl;
	}

	Settings::Chess960 = DFRC;
	Settings::Hash = 1;
	Settings::Threads = 1;
	StartTime = Clock::now();

	cout << "Datagen settings:" << endl;
	cout << " - " << Console::Yellow << randomPlyBase << Console::White << " or " << Console::Yellow << randomPlyBase + 1
		<< Console::White << " plies of random rollout, then normal playout" << endl;
	cout << " - Verification at depth " << Console::Yellow << verificationDepth << Console::White
		<< " with a threshold of " << Console::Yellow << startingEvalLimit << Console::White << endl;
	cout << " - Playing with a soft node limit of " << Console::Yellow << softNodeLimit << Console::White << endl;
	cout << " - Using DFRC starting positions: " << Console::Yellow << std::boolalpha << DFRC
		<< std::noboolalpha << Console::White << "\n" << endl;

	std::vector<std::thread> threads = std::vector<std::thread>();
	for (int i = 0; i < ThreadCount; i++) {
		std::string filenameForThread = filename + "_" + std::to_string(i + 1);
		threads.emplace_back(&Datagen::SelfPlay, this, filenameForThread);
	}
	for (std::thread& t : threads) t.join();
}

void Datagen::SelfPlay(const std::string filename) {

	Search* Searcher1 = new Search;
	Search* Searcher2 = new Search;
	Search* SearcherV = new Search;
	Searcher1->TranspositionTable.SetSize(1);
	Searcher2->TranspositionTable.SetSize(1);
	SearcherV->TranspositionTable.SetSize(1);
	Searcher1->SetThreadCount(1);
	Searcher2->SetThreadCount(1);
	SearcherV->SetThreadCount(1);
	Searcher1->DatagenMode = true;
	Searcher2->DatagenMode = true;
	SearcherV->DatagenMode = true;

	SearchParams params = SearchParams();
	params.softnodes = softNodeLimit;
	params.nodes = hardNodeLimit;
	params.depth = depthLimit;
	SearchParams verificationParams = SearchParams();
	verificationParams.depth = verificationDepth;
	
	int gamesOnThread = 0;
	std::mt19937 generator(std::random_device{}());

	std::vector<std::pair<std::string, int>> currentGame;
	std::vector<std::string> unsavedLines;


	while (true) {

		bool failed = false;
		int winAdjudicationCounter = 0;
		int drawAdjudicationCounter = 0;
		GameState outcome = GameState();
		currentGame.clear();

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

		// 2. Generate random moves from the start
		const int randomPlies = (gamesOnThread % 2 == 0) ? randomPlyBase : (randomPlyBase + 1);
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

		// Rarely, the last move will result in a checkmate position - filter these
		MoveList moves{};
		position.GenerateMoves(moves, MoveGen::All, Legality::Legal);
		if (moves.size() == 0) continue;

		// 3. Verify evaluation if acceptable
		const Results verificationResults = SearcherV->SearchSinglethreaded(position, verificationParams);
		SearcherV->ResetState(true);

		if (std::abs(verificationResults.score) > startingEvalLimit) failed = true;
		if (failed) continue;

		Searcher1->ResetState(true);
		Searcher2->ResetState(true);

		// 4. Play out the game
		while (true) {
			// Search
			Search* currentSearcher = (position.Turn() == Side::White) ? Searcher1 : Searcher2;
			const Results results = currentSearcher->SearchSinglethreaded(position, params);
			const Move move = results.BestMove();
			const int whiteScore = results.score * (position.Turn() == Side::Black ? -1 : 1);

			// Adjudication
			if (std::abs(whiteScore) > winAdjEvalThreshold) {
				winAdjudicationCounter += 1;
				if (winAdjudicationCounter >= winAdjEvalPlies) {
					outcome = (whiteScore > 0) ? GameState::WhiteVictory : GameState::BlackVictory;
					break;
				}
			}
			else winAdjudicationCounter = 0;

			if (std::abs(whiteScore) < drawAdjEvalThreshold) {
				winAdjudicationCounter = 0;
				if (drawAdjudicationCounter >= drawAdjPlies) {
					outcome = GameState::Draw;
					break;
				}
			}
			else drawAdjudicationCounter = 0;

			if (position.GetPly() > 600) {
				failed = true;
				//cout << "\nGame too long: " << board.GetFEN() << endl;
				break;
			}

			if (move.IsNull()) {
				failed = true;
				cout << "\nGot null-move for " << position.GetFEN() << " with eval of " << results.score << endl;
				break;
			}

			// Check if position should be stored
			PositionsTotal.fetch_add(1, std::memory_order_relaxed);
			if (!Filter(position, move, results.score)) {
				PositionsAccepted.fetch_add(1, std::memory_order_relaxed);
				currentGame.push_back(std::pair(position.GetFEN(), whiteScore));
			}
			position.Push(move);

			outcome = position.GetGameState();
			if (outcome != GameState::Playing) break;
		}

		if (failed) continue;

		Games.fetch_add(1, std::memory_order_relaxed);
		gamesOnThread += 1;

		// 5. Store the game to the memory, and periodically to the hard drive
		for (const auto& [fen, whiteScore] : currentGame) {
			unsavedLines.push_back(ToTextformat(fen, whiteScore, outcome));
		}
		if (gamesOnThread % 64 == 0) {
			std::ofstream file(filename, std::ios_base::app);
			const std::ostream_iterator<std::string> output_iterator(file, "\n");
			std::copy(std::begin(unsavedLines), std::end(unsavedLines), output_iterator);

			for (const auto& line : unsavedLines) file << line << '\n';
			file.close();
			unsavedLines.clear();
		}

		// 6. Update display
		if (Games.load(std::memory_order_relaxed) % 1000 == 0) {
			const auto endTime = Clock::now();
			const int seconds = static_cast<int>((endTime - StartTime).count() / 1e9);
			const int speed1 = PositionsAccepted.load(std::memory_order_relaxed) * 3600 / std::max(seconds, 1);
			const int speed2 = speed1 / 3600 / ThreadCount;

			const std::string display = "Games: " + std::to_string(Games.load(std::memory_order_relaxed))
				+ "  |  Positions: " + std::to_string(PositionsAccepted.load(std::memory_order_relaxed))
				+ "  |  Runtime: " + std::to_string(seconds) + "s  |  "
				+ std::to_string(speed1) + " per hour  (" + std::to_string(speed2) + "/s/th)";
			cout << display << endl; // '\r';
		}

	}
}

bool Datagen::Filter(const Position& pos, const Move& move, const int eval) const {
	if (std::abs(eval) > MateThreshold) return true;
	if (pos.GetPly() < minSavePly) return true;
	if (DFRC && move.IsCastling()) return true;
	if (!pos.IsMoveQuiet(move)) return true;
	if (pos.IsInCheck()) return true;
	return false;
}

std::string Datagen::ToTextformat(const std::string fen, const int16_t whiteScore, const GameState outcome) const {
	// Format: <fen> | <eval> | <wdl>
	// eval: white pov in cp, wdl 1.0 = white win, 0.0 = black win

	const std::string outcomeStr = [&] {
		switch (outcome) {
		case GameState::WhiteVictory: return "1.0";
		case GameState::BlackVictory: return "0.0";
		case GameState::Draw: return "0.5";
		default: return "???";
		}
	}();
	return fen + " | " + std::to_string(whiteScore) + " | " + outcomeStr;
}

void Datagen::MergeFiles() const {
	cout << "\nRenegade's file merging utility for datagen\n";
	const std::filesystem::path path = std::filesystem::current_path();
	cout << "Current folder: " << path << endl;

	std::string name;
	cout << "\nWhat is the base name of the generated files? ";
	cin >> name;

	uint64_t limit = -1;
	cout << "How many positions maximum (-1 for no limit)? ";
	cin >> limit;

	// Iterate through files in directory
	std::vector<std::string> found;
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		auto filename = entry.path().filename();
		if (filename.string().starts_with(name)) {
			found.push_back(filename.string());
		}
	}
	cout << "Found " << found.size() << " files\n" << endl;

	// Write file
	const std::string mergedName = name + "_merged";
	std::ofstream output;
	output.open(mergedName, std::ios_base::app);
	int counter = 0;

	for (const auto& filename : found) {
		std::ifstream ifs(filename);
		std::string line;

		while (std::getline(ifs, line)) {
			if (limit != -1 && counter >= limit) break;
			counter += 1;
			if (counter % 1'000'000 == 0) cout << ".";
			output << line << endl;
		}
		if (limit != -1 && counter >= limit) break;

		ifs.close();

		cout << filename << " -> processed: " << counter << endl;
	}

	output.close();
	cout << "\nComplete.\n" << endl;
}
