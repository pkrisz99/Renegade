#pragma once
#include <tuple>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Utils.h"

// General engine settings ------------------------------------------------------------------------

constexpr int HashMin = 1;
constexpr int HashDefault = 64;
constexpr int HashMax = 1048576;
constexpr int ThreadsMin = 1;
constexpr int ThreadsDefault = 1;
constexpr int ThreadsMax = 1024;
constexpr bool Chess960Default = false;
constexpr bool ShowWDLDefault = true;

namespace Settings {
	inline int Hash = HashDefault;
	inline int Threads = ThreadsDefault;
	inline bool ShowWDL = ShowWDLDefault;
	inline bool UseUCI = false;
	inline bool Chess960 = Chess960Default;
}

// Search parameter tuning ------------------------------------------------------------------------

struct Tunable {
	const std::string_view name;
	const int default_value;
	const int min;
	const int max;
	const int step;
	int value = default_value;
};

inline std::unordered_map<std::string_view, Tunable&> TunableParameterList;

#define ADD_TUNABLE(NAME, DEFAULT, MIN, MAX, STEP)                                         \
	inline Tunable NAME = Tunable{ #NAME, DEFAULT, MIN, MAX, STEP };                       \
	namespace Internal {                                                                   \
		struct RegisterTunable_##NAME {                                                    \
			RegisterTunable_##NAME() { TunableParameterList.emplace(NAME.name, NAME); }    \
		};                                                                                 \
		inline RegisterTunable_##NAME registerTunable_##NAME;                              \
	};                                                                                     \
	inline int tune_##NAME() { return NAME.value; }


inline void PrintTunableParameters() {
	for (const auto& [name, param] : TunableParameterList) {
		cout << "option name " << param.name << " type spin default " << param.default_value
			<< " min " << param.min << " max " << param.max << endl;
	}
}

inline void GenerateOpenBenchTuningString() {
	cout << "-------------------------------------------" << endl;
	cout << "Currently " << TunableParameterList.size() << " tunable values are plugged in:" << endl;
	cout << "-------------------------------------------" << endl;
	for (const auto& [name, param] : TunableParameterList) {
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

inline bool IsTuningActive() {
	return TunableParameterList.size() != 0;
}

inline bool HasTunableParameter(const std::string name) {
	return (TunableParameterList.find(name) != TunableParameterList.end());
}

inline void SetTunableParameter(const std::string name, const int value) {
	TunableParameterList.at(name).value = value;
}

// Add tunable parameters here --------------------------------------------------------------------
// Definition here: ADD_TUNABLE(name, default, min, max, step)
// Usage in code:   tune_name()

ADD_TUNABLE(rfp_margin, 103, 60, 120, 10)
ADD_TUNABLE(rfp_improving_reduction, 86, 60, 120, 15)

ADD_TUNABLE(nmp_eval_divider, 241, 150, 270, 30)

ADD_TUNABLE(fp_margin_offset, 45, 0, 100, 15)
ADD_TUNABLE(fp_margin_coeff, 112, 70, 150, 15)
ADD_TUNABLE(fp_margin_improving, 44, 10, 80, 15)

ADD_TUNABLE(lmr_multiplier, 40, 28, 45, 6)
ADD_TUNABLE(lmr_base, 76, 50, 100, 6)
ADD_TUNABLE(lmr_history_div, 20100, 13000, 26000, 2000)
ADD_TUNABLE(lmr_deeper_margin, 32, 25, 60, 7)

ADD_TUNABLE(asp_start, 12, 11, 17, 3)

ADD_TUNABLE(history_clamp, 2850, 1500, 3500, 300)
ADD_TUNABLE(history_coeff, 300, 150, 500, 30)
ADD_TUNABLE(history_cap, 14900, 8000, 30000, 1500)

ADD_TUNABLE(corrhist_cap, 8650, 6000, 10000, 800)
ADD_TUNABLE(corrhist_inertia, 194, 140, 300, 30)

ADD_TUNABLE(capthist_mul, 16, 10, 20, 3)
ADD_TUNABLE(capthist_div, 33, 16, 48, 6)

ADD_TUNABLE(ext_double, 23, 15, 24, 3)

ADD_TUNABLE(qsfp_margin, 274, 150, 450, 60)

ADD_TUNABLE(refut_killer, 22000, 8000, 40000, 5000)
ADD_TUNABLE(refut_counter, 16000, 4000, 32000, 4000)
ADD_TUNABLE(refut_positional, 16000, 4000, 32000, 4000)