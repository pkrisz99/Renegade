#pragma once
#include <tuple>
#include <string>
#include <string_view>
#include <unordered_map>
#include "Utils.h"

// General engine settings ------------------------------------------------------------------------

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

// Set tunable parameters here --------------------------------------------------------------------
// Format here is: ADD_TUNABLE(name, default, min, max, step)
// Usage in code:  tune_name()

ADD_TUNABLE(rfp_margin, 102, 60, 120, 10)
ADD_TUNABLE(rfp_improving_reduction, 90, 60, 120, 15)

ADD_TUNABLE(nmp_eval_divider, 211, 150, 270, 30)

ADD_TUNABLE(fp_margin_offset, 51, 0, 100, 15)
ADD_TUNABLE(fp_margin_coeff, 106, 70, 150, 15)

ADD_TUNABLE(lmr_multiplier, 35, 28, 45, 5)
ADD_TUNABLE(lmr_base, 78, 50, 100, 5)

ADD_TUNABLE(lmr_history_div, 19000, 13000, 26000, 2000)
ADD_TUNABLE(lmr_deeper_offset, 44, 30, 70, 8)
ADD_TUNABLE(lmr_deeper_coeff, 5, 3, 10, 1)

ADD_TUNABLE(asp_start, 16, 12, 20, 3)
ADD_TUNABLE(asp_widening, 128, 64, 256, 32)

ADD_TUNABLE(history_clamp, 2575, 1500, 3500, 300)
ADD_TUNABLE(history_coeff, 300, 150, 500, 30)
ADD_TUNABLE(history_cap, 15350, 8192, 32678, 1536)

ADD_TUNABLE(corrhist_cap, 7700, 6144, 9216, 768)
ADD_TUNABLE(corrhist_inertia, 242, 128, 512, 32)

ADD_TUNABLE(capthist_mul, 16, 12, 24, 4)
ADD_TUNABLE(capthist_div, 35, 16, 48, 4)

ADD_TUNABLE(ext_double, 30, 15, 30, 4)
