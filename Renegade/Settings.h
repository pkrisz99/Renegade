#pragma once
#include <tuple>
#include <string_view>
#include <unordered_map>
#include "Utils.h"

// General engine settings:

constexpr int HashMin = 1;
constexpr int HashDefault = 64;
constexpr int HashMax = 4096;

constexpr bool ShowWDLDefault = true;

namespace Settings {
	extern int Hash;
	extern bool ShowWDL;
	extern bool UseUCI;
}

// Search parameter tuning:

struct Tunable {
	std::string_view name;
	const int min;
	const int max;
	const int default_value;
	int step;
	int value = default_value;
};

namespace Tune {
	extern std::unordered_map<std::string_view, Tunable&> List;
	void PrintOptions();
	void GenerateString();
	bool Active();

	extern Tunable see_pawn, see_knight, see_bishop, see_rook, see_queen, see_quiet, see_noisy, see_search_th, see_qsearch_th;
}