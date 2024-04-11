#pragma once
#include <tuple>

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
// ...