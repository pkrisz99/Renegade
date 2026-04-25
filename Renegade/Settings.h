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

ADD_TUNABLE(rfp_margin, 115, 70, 150, 12)
ADD_TUNABLE(rfp_improving_reduction, 73, 50, 120, 12)

ADD_TUNABLE(nmp_eval_divider, 259, 150, 350, 40)

ADD_TUNABLE(fp_margin_base, 48, 0, 100, 15)
ADD_TUNABLE(fp_margin_coeff, 100, 70, 150, 15)
ADD_TUNABLE(fp_margin_improving, 53, 10, 80, 15)

ADD_TUNABLE(lmr_multiplier, 45, 30, 60, 5)
ADD_TUNABLE(lmr_base, 76, 50, 100, 6)
ADD_TUNABLE(lmr_history_div, 22100, 15000, 30000, 2500)
ADD_TUNABLE(lmr_deeper_margin, 30, 23, 45, 5)

ADD_TUNABLE(asp_start, 12, 10, 14, 2)

ADD_TUNABLE(history_quiet_clamp, 3160, 2000, 4000, 300)
ADD_TUNABLE(history_quiet_coeff, 302, 150, 500, 30)
ADD_TUNABLE(history_quiet_cap, 16600, 8000, 30000, 1500)
ADD_TUNABLE(history_noisy_clamp, 3160, 2000, 4000, 300)
ADD_TUNABLE(history_noisy_coeff, 302, 150, 500, 30)
ADD_TUNABLE(history_noisy_cap, 16600, 8000, 30000, 1500)

ADD_TUNABLE(corrhist_cap, 9710, 7000, 14000, 1200)
ADD_TUNABLE(corrhist_inertia, 143, 80, 300, 25)
ADD_TUNABLE(corrhist_depth_weight, 16, 9, 22, 3)

ADD_TUNABLE(capthist_mul, 18, 13, 25, 3)
ADD_TUNABLE(capthist_div, 28, 16, 48, 5)

ADD_TUNABLE(ext_double, 23, 16, 24, 3)
ADD_TUNABLE(ext_triple, 200, 50, 230, 30)

ADD_TUNABLE(qsfp_margin, 267, 150, 450, 50)

ADD_TUNABLE(refut_killer, 17700, 8000, 40000, 4000)
ADD_TUNABLE(refut_counter, 14000, 4000, 32000, 4000)
ADD_TUNABLE(refut_positional, 16300, 4000, 32000, 4000)

ADD_TUNABLE(hp_coeff, 8000, 2000, 20000, 2500)