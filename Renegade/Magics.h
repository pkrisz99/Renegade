#pragma once
#include <random>
#include "Utils.cpp"

struct RookMagicBits {
	std::array<uint64_t, 4096> Occupancies;
	std::array<uint64_t, 4096> Attacks;
};

struct BishopMagicBits {
	std::array<uint64_t, 512> Occupancies;
	std::array<uint64_t, 512> Attacks;
};

static std::array<uint64_t, 64> RookMasks;
static std::array<uint64_t, 64> BishopMasks;
static std::array<int, 64> RookRelevantBits;
static std::array<int, 64> BishopRelevantBits;
static std::vector<RookMagicBits> RookMagics(64);
static std::array<uint64_t, 64> RookMagicNumbers;
static std::vector<BishopMagicBits> BishopMagics(64);
static std::array<uint64_t, 64> BishopMagicNumbers;

void GenerateMagics();
uint64_t GetRookAttacks(const int square, const uint64_t occupancy);
uint64_t GetBishopAttacks(const int square, const uint64_t occupancy);
void DebugBitboard(const uint64_t bits);
uint64_t BishopAttacks(const int square, const uint64_t blockers);
uint64_t GenerateMagicOccupancy(const int encoded, uint64_t mask);
uint64_t RookAttacks(const int square, const uint64_t blockers);