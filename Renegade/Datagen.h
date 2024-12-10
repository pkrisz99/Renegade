#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <thread>
#include "Position.h"
#include "Search.h"
#include "Settings.h"
#include "Utils.h"


// Datagen settings:
constexpr int startingEvalLimit = 500;
constexpr int verificationDepth = 10;
constexpr int softNodeLimit = 5000;
constexpr int hardNodeLimit = 500000;
constexpr int depthLimit = 20;
constexpr int randomPlyBaseNormal = 2;
constexpr int randomPlyBaseDFRC = 4;
constexpr int minSavePly = 16;

constexpr int drawAdjEvalThreshold = 5;
constexpr int drawAdjPlies = 15;
constexpr int winAdjEvalThreshold = 2000;
constexpr int winAdjEvalPlies = 5;

enum class DatagenLaunchMode { Ask, Normal, DFRC };

void MergeDatagenFiles();
void StartDatagen(const DatagenLaunchMode launchMode);
