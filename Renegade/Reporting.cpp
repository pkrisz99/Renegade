#include "Reporting.h"

// Result structure -------------------------------------------------------------------------------

Results::Results() {
	score = 0;
	pv = std::vector<Move>();
	time = 0;
	nps = 0;
	hashfull = 0;
}

Results::Results(int s, std::vector<Move> p, int d, SearchStatistics st, int t, int speed, int h, int pl) {
	score = s;
	pv = p;
	depth = d;
	stats = st;
	time = t;
	nps = speed;
	hashfull = h;
	ply = pl;
}

Move Results::BestMove() {
	if (pv.size() != 0) return pv.front();
	else return Move();
}

// Communicating the search results ---------------------------------------------------------------

void PrintInfo(const Results& e, const EngineSettings& settings) {
	if (!settings.UciOutput) {
		PrintPretty(e, settings);
		return;
	}

	const int normalizedScore = ToCentipawns(e.score, e.ply);

	std::string score;
	if (IsMateScore(e.score)) {
		int movesToMate = (MateEval - std::abs(e.score) + 1) / 2;
		if (e.score > 0) score = "mate " + std::to_string(movesToMate);
		else score = "mate -" + std::to_string(movesToMate);
	}
	else {
		score = "cp " + std::to_string(normalizedScore);
	}

	/*
	std::string extended;
	if (settings.ExtendedOutput) {
		extended += " evals " + std::to_string(e.stats.Evaluations);
		extended += " qnodes " + std::to_string(e.stats.QuiescenceNodes);
		// extended += " betacutoffs " + std::to_string(e.stats.BetaCutoffs)
		// extended += " fmbc " + std::to_string(e.stats.FirstMoveBetaCutoffs);
		if (e.stats.BetaCutoffs != 0) extended += " fmbcrate " + std::to_string(static_cast<int>(e.stats.FirstMoveBetaCutoffs * 1000 / e.stats.BetaCutoffs));
		extended += " tthits " + std::to_string(e.stats.TranspositionHits);
		if (e.stats.TranspositionQueries != 0) extended += " ttrate " + std::to_string(static_cast<int>(e.stats.TranspositionHits * 1000 / e.stats.TranspositionQueries));
	}*/

	std::string pvString;
	for (const Move& move : e.pv)
		pvString += " " + move.ToString();

	std::string output{};
	std::string wdlOutput{};

	if (settings.ShowWDL) {
		const auto [w, l] = GetWDL(e.score, e.ply);
		const int d = 1000 - w - l;
		wdlOutput = " wdl " + std::to_string(w) + " " + std::to_string(d) + " " + std::to_string(l);
	}

#if defined(_MSC_VER)
	output = std::format("info depth {} seldepth {} score {}{} nodes {} nps {} time {} hashfull {} pv{}",
		e.depth, e.stats.SelDepth, score, wdlOutput, e.stats.Nodes, e.nps, e.time, e.hashfull, pvString);

#else
	output = "info depth " + std::to_string(e.depth) + " seldepth " + std::to_string(e.stats.SelDepth)
		+ " score " + score + wdlOutput + " nodes " + std::to_string(e.stats.Nodes) + " nps " + std::to_string(e.nps)
		+ " time " + std::to_string(e.time) + " hashfull " + std::to_string(e.hashfull)
		+ " pv" + pvString;
#endif

	cout << output << endl;
}

// Printing current search results ----------------------------------------------------------------

void PrintPretty(const Results& e, const EngineSettings& settings) {
#if defined(_MSC_VER)
	const std::string green = settings.Colorful ? "\x1b[92m" : "";
	const std::string blue = settings.Colorful ? "\x1b[96m" : "";
	const std::string red = settings.Colorful ? "\x1b[91m" : "";
	const std::string white = settings.Colorful ? "\x1b[0m" : "";
	const std::string gray = settings.Colorful ? "\x1b[90m" : "";
	const std::string yellow = settings.Colorful ? "\x1b[93m" : "";

	const int normalizedScore = ToCentipawns(e.score, e.ply);

	std::string output1 = std::format("{} {:2d}/{:2d}",
		white, e.depth, e.stats.SelDepth);

	std::string nodeString;
	if (e.stats.Nodes < 1e9) nodeString = std::to_string(e.stats.Nodes);
	else nodeString = std::format("{:8.3f}", e.stats.Nodes / 1e9) + "b";

	std::string timeString;
	if (e.time < 60000) timeString = std::to_string(e.time) + "ms";
	else {
		int seconds = e.time / 1000;
		int minutes = seconds / 60;
		seconds = seconds - minutes * 60;
		timeString = std::format("{}m{:02d}s", minutes, seconds);
	}

	std::string hashString;
	if (e.hashfull != 1000) hashString = std::format("{:4.1f}", e.hashfull / 10.0);
	else hashString = "100";

	std::string output3 = std::format("{}  {:>9}  {:>7}  {:4d}knps  h={:>4}% {} -> ",
		gray, nodeString, timeString, e.nps / 1000, hashString, white);

	const int neutralMargin = 50;
	std::string scoreColor = blue;
	if (!IsMateScore(normalizedScore)) {
		if (normalizedScore > neutralMargin) scoreColor = green;
		else if (normalizedScore < -neutralMargin) scoreColor = red;
	}
	else {
		scoreColor = yellow;
	}

	std::string output2;
	if (!IsMateScore(normalizedScore))
		output2 = std::format("{} {:+5.2f}  {}", scoreColor, normalizedScore / 100.0, white);
	else {
		std::string output2mate;
		int movesToMate = (MateEval - std::abs(normalizedScore) + 1) / 2;
		if (normalizedScore > 0) output2mate = "#+" + std::to_string(movesToMate);
		else output2mate = "#-" + std::to_string(movesToMate);
		output2 = std::format("{} {:5}  {}", scoreColor, output2mate, white);
	}

	std::string pvOutput;
	for (int i = 0; i < 5; i++) {
		if (i >= static_cast<int>(e.pv.size())) break;
		pvOutput += e.pv.at(i).ToString() + " ";
	}
	if (pvOutput.length() != 0) pvOutput.pop_back();
	if (e.pv.size() > 5) pvOutput += " (+" + std::to_string(e.pv.size() - 5) + ")";
	if (e.pv.size() != 0) pvOutput = white + "[" + pvOutput + white + "]";

	// Win-draw-loss
	const auto [modelW, modelL] = GetWDL(e.score, e.ply);
	const int modelD = 1000 - modelW - modelL;
	const double q = 10.0;
	int w = static_cast<int>(std::round(modelW / q));
	int d = static_cast<int>(std::round(modelD / q));
	int l = static_cast<int>(std::round(modelL / q));
	std::string wdl = gray + std::to_string(w) + "-" + std::to_string(d) + "-" + std::to_string(l) + gray;

	std::string output = output1 + output3 + output2 + wdl + "  " + pvOutput + white;

	cout << output << endl;
#endif
}

void PrintBestmove(const Move& move) {
	cout << "bestmove " << move.ToString() << endl;
}