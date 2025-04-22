#include "Utils.h"

// https://stackoverflow.com/questions/25829143/trim-whitespace-from-a-string
std::string Trim(const std::string& str) {
	const size_t first = str.find_first_not_of(' ');
	const size_t last = str.find_last_not_of(' ');
	if (first == std::string::npos) return "";
	return str.substr(first, (last - first + 1));
}

std::vector<std::string> Split(const std::string& cmd) {
	std::stringstream ss(cmd);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	return std::vector<std::string>(begin, end);
}

// https://stackoverflow.com/questions/8095088/how-to-check-string-start-in-c
bool StartsWith(const std::string& big, const std::string& small) {
	return big.compare(0, small.length(), small) == 0;
}

void ConvertToLowercase(std::string& str) {
	for (size_t x = 0; x < str.length(); x++) str[x] = std::tolower(str[x]);
}

[[maybe_unused]] void PrintBitboard(const uint64_t bits) {
	cout << "\n" << bits << '\n';
	for (int r = 7; r >= 0; r--) {
		for (int f = 0; f < 8; f++) {
			if (CheckBit(bits, Square(r, f))) cout << " X ";
			else cout << " . ";
		}
		cout << "\n";
	}
	cout << "\n" << endl;
}

// WDL model and reporting ------------------------------------------------------------------------

// Getting the model for a given game ply
std::pair<double, double> ModelWDLForPly(const int ply) {
	const double m = std::min(240.0, static_cast<double>(ply)) / 64.0;

	return {
		(((as[0] * m + as[1]) * m + as[2]) * m) + as[3],
		(((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3]
	};
}

// Get win, draw and loss probabilities per mil for a given internal score and ply
std::tuple<int, int, int> GetWDL(const int score, const int ply) {
	const auto [a, b] = ModelWDLForPly(ply);
	const double x = std::clamp(static_cast<double>(score), -4000.0, 4000.0);

	const double w = std::round(1000.0 / (1.0 + std::exp((a - x) / b)));
	const double l = std::round(1000.0 / (1.0 + std::exp((a + x) / b)));
	const double d = 1000.0 - w - l;

	return {
		static_cast<int>(std::round(w)),
		static_cast<int>(std::round(d)),
		static_cast<int>(std::round(l))
	};
}

// Converts internal units into centipawns, following the convention of 100 cp = 50% chance of winning
int ToCentipawns(const int score, const int ply) {
	if ((std::abs(score) >= MateThreshold) || (score == 0)) return score;
	const auto [a, b] = ModelWDLForPly(ply);
	return static_cast<int>(std::round(100.0 * static_cast<double>(score) / a));
}