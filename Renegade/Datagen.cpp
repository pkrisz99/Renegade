#include "Datagen.h"

static void SetTitle(const std::string title) {
	Console::ClearScreen();
	cout << Console::Highlight << " " << title << " " << Console::White;
	cout << " [" << Version << "]\n" << endl;
}

static void PressEnterToExit() {
	cout << "Press enter to exit." << endl;
	cin.ignore();
	cin.get();
}

// Self-play datagen ------------------------------------------------------------------------------ 

void SelfPlay(const std::string filename);  // main loop per thread, forward declaring this

std::atomic<uint64_t> PositionsAccepted = 0, PositionsTotal = 0;
std::atomic<uint64_t> Games = 0, Plies = 0, Searches = 0, Depths = 0, Nodes = 0;
std::atomic<uint64_t> WhiteWins = 0, Draws = 0, BlackWins = 0;
Clock::time_point StartTime;
int ThreadCount = 0;
bool DFRC = false;
std::vector<std::string> Openings{};

static std::string FormatRuntime(const int seconds) {
	const auto [minutes, s] = std::div(seconds, 60);
	const auto [h, m] = std::div(minutes, 60);
	//return std::format("{}:{:02d}:{:02d}", h, m, s);
	std::stringstream oss;
	oss << h << ':' << std::setw(2) << std::setfill('0') << m << ':' << std::setw(2) << std::setfill('0') << s;
	return oss.str();
}

static bool Filter(const Position& pos, const Move& move, const int eval) {
	if (std::abs(eval) > MateThreshold) return true;
	if (pos.GetPly() < minSavePly) return true;
	if (DFRC && move.IsCastling()) return true;
	if (!pos.IsMoveQuiet(move)) return true;
	if (pos.IsInCheck()) return true;
	return false;
}

static std::string ToTextformat(const std::string fen, const int16_t whiteScore, const GameState outcome) {
	// Format: <fen> | <eval> | <wdl>
	// eval: white pov in cp, wdl 1.0 = white win, 0.0 = black win

	const std::string outcomeStr = [&] {
		switch (outcome) {
		case GameState::WhiteVictory: return "1.0";
		case GameState::BlackVictory: return "0.0";
		case GameState::Drawn: return "0.5";
		default: return "???";
		}
		}();
	return fen + " | " + std::to_string(whiteScore) + " | " + outcomeStr;
}

void StartDatagen(const DatagenLaunchMode launchMode) {

	// There is a streamlined way to launch datagen (normal and dfrc) from the command line using the default settings
	// When launched from UCI, the settings has to be provided manually

	SetTitle("Renegade's datagen utility");
	std::string filename;

	if (launchMode == DatagenLaunchMode::Ask) {
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
		DFRC = launchMode == DatagenLaunchMode::DFRC;
		
		cout << "Quick start details:" << endl;
		cout << " - Filename: " << Console::Yellow << filename << Console::White << endl;
		cout << " - Thread count: " << Console::Yellow << ThreadCount << Console::White << endl;
		cout << " - Doing DFRC: " << Console::Yellow << DFRC << Console::White << '\n' << endl;
	}

	// Load opening book - using these seem to improve net strength
	if (!DFRC) {
		const std::string book = "UHO_Lichess_4852_v1.epd";
		std::ifstream bookFile(book);
		std::string line;
		while (std::getline(bookFile, line)) Openings.push_back(line);

		if (Openings.size() == 0) {
			cout << Console::Red << "For non-DFRC datagen it is required to have the " << book << " book." << Console::White << endl;
			PressEnterToExit();
			return;
		}
	}

	Settings::Chess960 = DFRC;
	Settings::Hash = 1;
	Settings::Threads = 1;
	StartTime = Clock::now();

	cout << "Datagen settings:" << endl;
	if (!DFRC) cout << " - After the book exit do " << Console::Yellow << randomPlyBaseNormal << Console::White << " plies of random moves, then play normally" << endl;
	else cout << " - " << Console::Yellow << randomPlyBaseDFRC << Console::White << " or " << Console::Yellow << randomPlyBaseDFRC + 1
		<< Console::White << " plies of random rollout, then normal playout" << endl;
	cout << " - Verification at depth " << Console::Yellow << verificationDepth << Console::White
		<< " with a threshold of " << Console::Yellow << startingEvalLimit << Console::White << endl;
	cout << " - Playing with a soft node limit of " << Console::Yellow << softNodeLimit << Console::White << endl;
	cout << " - Using DFRC starting positions: " << Console::Yellow << std::boolalpha << DFRC
		<< std::noboolalpha << Console::White << endl;
	if (!DFRC) cout << " - The opening book has " << Console::Yellow << Console::FormatInteger(Openings.size()) << Console::White << " lines" << endl;
	cout << endl;

	std::vector<std::thread> threads = std::vector<std::thread>();
	for (int i = 0; i < ThreadCount; i++) {
		std::string filenameForThread = filename + "_" + std::to_string(i + 1);
		threads.emplace_back(&SelfPlay, filenameForThread);
	}
	for (std::thread& t : threads) t.join();
}

void SelfPlay(const std::string filename) {

	Search* Searcher1 = new Search;
	Search* Searcher2 = new Search;
	Search* SearcherV = new Search;
	Searcher1->TranspositionTable.SetSize(1, 1);
	Searcher2->TranspositionTable.SetSize(1, 1);
	SearcherV->TranspositionTable.SetSize(1, 1);
	Searcher1->SetThreadCount(1);
	Searcher2->SetThreadCount(1);
	SearcherV->SetThreadCount(1);

	SearchParams params = SearchParams();
	params.softnodes = softNodeLimit;
	params.nodes = hardNodeLimit;
	params.depth = depthLimit;
	SearchParams verificationParams = SearchParams();
	verificationParams.depth = verificationDepth;
	std::uniform_int_distribution<std::size_t> nodesDistribution(softNodeLimit - 100, softNodeLimit + 100);
	const int randomPlyBase = DFRC ? randomPlyBaseDFRC : randomPlyBaseNormal;
	
	int gamesOnThread = 0;
	std::mt19937 generator(std::random_device{}());

	std::vector<std::pair<std::string, int>> currentGame{};
	std::vector<std::string> unsavedLines{};


	while (true) {

		bool failed = false;
		int winAdjudicationCounter = 0;
		int drawAdjudicationCounter = 0;
		GameState outcome = GameState();
		currentGame.clear();

		// 1. Reset state
		Position position = [&] {
			if (!DFRC) {
				std::uniform_int_distribution<std::size_t> distribution(0, Openings.size() - 1);
				const std::string opening = Openings[distribution(generator)];
				return Position(opening);
			}
			else {
				std::uniform_int_distribution<std::size_t> distribution(0, 959);
				const int whiteFrcIndex = distribution(generator);
				const int blackFrcIndex = distribution(generator);
				return Position(whiteFrcIndex, blackFrcIndex);
			}
		}();

		// 2. Generate random moves from the start
		const int randomPlies = [&] {
			if (!DFRC) return randomPlyBase;
			else return (gamesOnThread % 2 == 0) ? randomPlyBase : (randomPlyBase + 1);
		}();

		for (int i = 0; i < randomPlies; i++) {
			MoveList moves{};
			position.GenerateMoves(moves, MoveGen::All, Legality::Legal);
			if (moves.size() == 0) {
				failed = true;
				break;
			}
			std::uniform_int_distribution<std::size_t> distribution(0, moves.size() - 1);
			position.PushMove(moves[distribution(generator)].move);
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
			SearchParams currentParams = params;
			currentParams.softnodes = nodesDistribution(generator);
			Search* currentSearcher = (position.Turn() == Side::White) ? Searcher1 : Searcher2;
			const Results results = currentSearcher->SearchSinglethreaded(position, currentParams);
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
				drawAdjudicationCounter += 1;
				if (drawAdjudicationCounter >= drawAdjPlies) {
					outcome = GameState::Drawn;
					break;
				}
			}
			else drawAdjudicationCounter = 0;

			if (position.GetPly() > 600) {
				failed = true;
				break;
			}

			if (move.IsNull()) {
				failed = true;
				cout << Console::Yellow << "Got null-move for " << position.GetFEN() << " with eval of " << results.score << Console::White << endl;
				break;
			}

			// Check if position should be stored
			PositionsTotal.fetch_add(1, std::memory_order_relaxed);
			Searches.fetch_add(1, std::memory_order_relaxed);
			Depths.fetch_add(results.depth, std::memory_order_relaxed);
			Nodes.fetch_add(results.nodes, std::memory_order_relaxed);
			
			if (!Filter(position, move, results.score)) {
				PositionsAccepted.fetch_add(1, std::memory_order_relaxed);
				currentGame.push_back(std::pair(position.GetFEN(), whiteScore));
			}
			position.PushMove(move);

			outcome = position.GetGameState();
			if (outcome != GameState::Playing) break;
		}

		if (failed) continue;

		gamesOnThread += 1;
		Games.fetch_add(1, std::memory_order_relaxed);
		Plies.fetch_add(position.GetPly(), std::memory_order_relaxed);

		if (outcome == GameState::WhiteVictory) WhiteWins.fetch_add(1, std::memory_order_relaxed);
		else if (outcome == GameState::Drawn) Draws.fetch_add(1, std::memory_order_relaxed);
		else if (outcome == GameState::BlackVictory) BlackWins.fetch_add(1, std::memory_order_relaxed);
		
		// 5. Store the game to the memory, and periodically to the hard drive
		for (const auto& [fen, whiteScore] : currentGame) {
			unsavedLines.push_back(ToTextformat(fen, whiteScore, outcome));
		}
		if (gamesOnThread % 64 == 0) {
			std::ofstream file(filename, std::ios_base::app);
			const std::ostream_iterator<std::string> output_iterator(file, "\n");
			std::copy(std::begin(unsavedLines), std::end(unsavedLines), output_iterator);
			file.close();
			unsavedLines.clear();
		}

		// 6. Update display
		const int games = Games.load(std::memory_order_relaxed);
		if (games % 100 == 0) {
			const auto endTime = Clock::now();
			const int seconds = static_cast<int>((endTime - StartTime).count() / 1e9);
			const int speed1 = PositionsAccepted.load(std::memory_order_relaxed) * 3600 / std::max(seconds, 1);
			const int speed2 = speed1 / 3600 / ThreadCount;

			const std::string display = "Games: " + Console::FormatInteger(games)
				+ " | Positions: " + Console::FormatInteger(PositionsAccepted.load(std::memory_order_relaxed))
				+ " | Runtime: " + FormatRuntime(seconds)
				+ " | Speed: " + std::to_string(speed1 / 1000) + "k/h " + std::to_string(speed2) + "/s/th";
			cout << display << endl;

#ifdef __cpp_lib_format
			if (games % 1000 == 0) {
				const float whiteWinRate = WhiteWins.load(std::memory_order_relaxed) * 100.f / games;
				const float drawRate = Draws.load(std::memory_order_relaxed) * 100.f / games;
				const float blackWinRate = BlackWins.load(std::memory_order_relaxed) * 100.f / games;
				const float avgPlies = static_cast<float>(Plies.load(std::memory_order_relaxed)) / games;
				const int searches = Searches.load(std::memory_order_relaxed);
				const int avgNodes = Nodes.load(std::memory_order_relaxed) / searches;
				const float avgDepths = static_cast<float>(Depths.load(std::memory_order_relaxed)) / searches;

				const std::string outcomeString = std::format("Outcomes: {:.1f}% - {:.1f}% - {:.1f}%", whiteWinRate, drawRate, blackWinRate);
				const std::string lengthString = std::format("Avg. game length: {:.1f} plies", avgPlies);
				const std::string depthString = std::format("Avg. depth: {:.2f}", avgDepths);
				const std::string nodesString = std::format("Avg. nodes: {}", avgNodes);

				cout << Console::Gray;
				cout << std::format(" -> {:32}  -> {:32}\n", outcomeString, lengthString);
				cout << std::format(" -> {:32}  -> {:32}\n", depthString, nodesString);
				cout << Console::White << std::flush;
			}
#endif
		}

	}
}

// File merging tool ------------------------------------------------------------------------------

void MergeDatagenFiles() {
	SetTitle("Renegade's file merging utility for datagen");
	const std::filesystem::path path = std::filesystem::current_path();
	cout << "Current folder: " << Console::Yellow << path.string() << Console::White << endl;

	std::string name;
	cout << "\nWhat is the base name of the generated files? " << Console::Yellow;
	cin >> name;

	int64_t limit = -1;
	cout << Console::White << "How many positions maximum (-1 for no limit)? " << Console::Yellow;
	cin >> limit;

	// Iterate through files in directory
	std::vector<std::string> found;
	for (const auto& entry : std::filesystem::directory_iterator(path)) {
		auto filename = entry.path().filename();
		if (filename.string().starts_with(name)) {
			found.push_back(filename.string());
		}
	}
	cout << Console::White << "\nFound " << Console::Yellow << found.size() << Console::White << " files\n" << endl;

	// Write file
	const std::string mergedName = name + "_merged";
	std::ofstream output;
	output.open(mergedName, std::ios_base::app);
	int64_t counter = 0;

	for (const auto& filename : found) {
		std::ifstream ifs(filename);
		std::string line;
		cout << filename << ": ";

		while (std::getline(ifs, line)) {
			if (limit != -1 && counter >= limit) break;
			counter += 1;
			if (counter % 1'000'000 == 0) cout << ".";

			// Basic check if the output was okay
			const int pipeCount = std::ranges::count(line, '|');
			if (pipeCount != 2) {
				cout << "\n-> Malformed: '" << line << "'" << endl;
			}

			// To do: collect stats and display them

			if (counter % 100 == 0) output << line << endl;
			else output << line << '\n';
		}
		if (limit != -1 && counter >= limit) break;

		ifs.close();

		cout << endl;
		cout << " -> processed: " << Console::FormatInteger(counter) << endl;
	}

	output.close();
	cout << Console::Green << "\nCompleted." << Console::White << endl;
	PressEnterToExit();
}
