#pragma once
#include <tuple>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Utils.h"

// General engine settings:

constexpr int HashMin = 1;
constexpr int HashDefault = 64;
constexpr int HashMax = 65536;
constexpr int ThreadsMin = 1;
constexpr int ThreadsDefault = 1;
constexpr int ThreadsMax = 256;
constexpr bool Chess960Default = false;
constexpr bool ShowWDLDefault = true;

namespace Settings {
	inline int Hash = HashDefault;
	inline int Threads = ThreadsDefault;
	inline bool ShowWDL = ShowWDLDefault;
	inline bool UseUCI = false;
	inline bool Chess960 = Chess960Default;
}

// Search parameter tuning:

struct Tunable {
	const std::string_view name;
	const int default_value;
	const int min;
	const int max;
	const int step;
	int value = default_value;
};

#define ADD_TUNABLE(NAME, DEFAULT, MIN, MAX, STEP)                         \
	inline Tunable NAME = Tunable{ #NAME, DEFAULT, MIN, MAX, STEP };       \
	struct RegisterTunable_##NAME {                                        \
		RegisterTunable_##NAME() { List.emplace(NAME.name, NAME); }  \
	};                                                                     \
	inline RegisterTunable_##NAME registerTunable_##NAME;                  \
    inline int tune_##NAME() { return NAME.value; }

namespace Tune {

	inline std::unordered_map<std::string_view, Tunable&> List;

	// === Add parameters to be tuned here ===
	// Format is: ADD_TUNABLE(name, default, min, max, step)

	ADD_TUNABLE(rfp_margin,              95, 60, 120, 10)
	ADD_TUNABLE(rfp_improving_reduction, 85, 60, 120, 10)

	ADD_TUNABLE(nmp_eval_divider, 200, 100, 300, 30)

	ADD_TUNABLE(fp_margin_offset, 30, 0, 100, 15)
	ADD_TUNABLE(fp_margin_coeff, 100, 70, 150, 15)

	ADD_TUNABLE(lmr_multiplier, 40, 30, 55, 5)
	ADD_TUNABLE(lmr_base, 70, 40, 100, 5)
	ADD_TUNABLE(lmr_history_div, 16384, 8192, 32678, 2048)
	ADD_TUNABLE(lmr_deeper_offset, 50, 30, 70, 8)
	ADD_TUNABLE(lmr_deeper_coeff, 5, 3, 10, 1)

	ADD_TUNABLE(asp_start, 20, 12, 26, 3)

	ADD_TUNABLE(history_clamp, 2550, 1000, 3000, 150)
	ADD_TUNABLE(history_coeff, 300, 150, 500, 25)
	ADD_TUNABLE(history_cap, 16384, 8192, 32678, 1024)

	ADD_TUNABLE(corrhist_cap, 6144, 4096, 8192, 512)
	ADD_TUNABLE(corrhist_inertia, 256, 128, 512, 32)

	ADD_TUNABLE(capthist_mul, 16, 8, 24, 4)
	ADD_TUNABLE(capthist_div, 32, 16, 48, 4)
	

	// =======================================	

	inline void PrintParameters() {
		for (const auto& [name, param] : List) {
			cout << "option name " << param.name << " type spin default " << param.default_value << " min " << param.min << " max " << param.max << endl;
		}
	}

	inline void GenerateOpenBenchString() {
		cout << "-------------------------------------------" << endl;
		cout << "Currently " << List.size() << " tunable values are plugged in:" << endl;
		for (const auto& [name, param] : List) {
			cout << param.name << ", ";
			cout << "int" << ", ";
			cout << param.default_value << ", ";
			cout << param.min << ", ";
			cout << param.max << ", ";
			cout << param.step << ", ";
			cout << "0.002 " << endl;
		}
		cout << "-------------------------------------------" << endl;
	}

	inline bool IsActive() {
		return List.size() != 0;
	}

	inline bool HasParameter(const std::string name) {
		return (List.find(name) != List.end());
	}

	inline void SetParameter(const std::string name, const int value) {
		List.at(name).value = value;
	}

}
