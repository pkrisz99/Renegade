#include "Magics.h"

// Largely based on https://github.com/maksimKorzh/chess_programming/blob/master/src/magics/magics.c
// and indirectly on https://www.chessprogramming.org/Looking_for_Magics#Feeding_in_Randoms

// Retrieving attack bitboards --------------------------------------------------------------------

uint64_t GetRookAttacks(const int square, const uint64_t occupancy) {
	int index = static_cast<int>(((occupancy & RookMasks[square]) * RookMagicNumbers[square]) >> (64 - RookRelevantBits[square]));
	return RookAttacks[square][index];
}

uint64_t GetBishopAttacks(const int square, const uint64_t occupancy) {
	int index = static_cast<int>(((occupancy & BishopMasks[square]) * BishopMagicNumbers[square]) >> (64 - BishopRelevantBits[square]));
	return BishopAttacks[square][index];
}

uint64_t GetQueenAttacks(const int square, const uint64_t occupancy) {
	int rookIndex = static_cast<int>(((occupancy & RookMasks[square]) * RookMagicNumbers[square]) >> (64 - RookRelevantBits[square]));
	int bishopIndex = static_cast<int>(((occupancy & BishopMasks[square]) * BishopMagicNumbers[square]) >> (64 - BishopRelevantBits[square]));
	return BishopAttacks[square][bishopIndex] | RookAttacks[square][rookIndex];
}

// Things for lookup table population -------------------------------------------------------------

// Generate an occupancy bitboard where the encoded bits represent occupied bits in the mask
uint64_t GenerateMagicOccupancy(const int encoded, uint64_t mask) {
	uint64_t map = 0;
	int popcount = Popcount(mask);
	int i = popcount - 1;
	while (Popcount(mask) != 0) {
		uint64_t sq = 63ULL - Lzcount(mask);
		if (CheckBit(static_cast<uint64_t>(encoded), i)) SetBitTrue(map, sq);
		SetBitFalse(mask, sq);
		i -= 1;
	}
	return map;
}

// Dynamically generate rook attacks
uint64_t DynamicRookAttacks(const int square, const uint64_t blockers) {
	uint64_t mask = 0;
	int file = GetSquareFile(square);
	int rank = GetSquareRank(square);

	for (int r = rank + 1; r < 8; r++) {
		SetBitTrue(mask, Square(r, file));
		if (CheckBit(blockers, Square(r, file))) break;
	}
	for (int r = rank - 1; r >= 0; r--) {
		SetBitTrue(mask, Square(r, file));
		if (CheckBit(blockers, Square(r, file))) break;
	}
	for (int f = file + 1; f < 8; f++) {
		SetBitTrue(mask, Square(rank, f));
		if (CheckBit(blockers, Square(rank, f))) break;
	}
	for (int f = file - 1; f >= 0; f--) {
		SetBitTrue(mask, Square(rank, f));
		if (CheckBit(blockers, Square(rank, f))) break;
	}

	return mask;
}

// Dynamically generate bishop attacks
uint64_t DynamicBishopAttacks(const int square, const uint64_t blockers) {
	uint64_t mask = 0;
	int file = GetSquareFile(square);
	int rank = GetSquareRank(square);

	// Ray towards bottom left
	int r = rank - 1; int f = file - 1;
	while ((r >= 0) && (r <= 7) && (f >= 0) && (f <= 7)) {
		SetBitTrue(mask, Square(r, f));
		if (CheckBit(blockers, Square(r, f))) break;
		r -= 1; f -= 1;
	}
	// Ray towards bottom right
	r = rank - 1; f = file + 1;
	while ((r >= 0) && (r <= 7) && (f >= 0) && (f <= 7)) {
		SetBitTrue(mask, Square(r, f));
		if (CheckBit(blockers, Square(r, f))) break;
		r -= 1; f += 1;
	}
	// Ray towards top right
	r = rank + 1; f = file + 1;
	while ((r >= 0) && (r <= 7) && (f >= 0) && (f <= 7)) {
		SetBitTrue(mask, Square(r, f));
		if (CheckBit(blockers, Square(r, f))) break;
		r += 1; f += 1;
	}
	// Ray towards top left
	r = rank + 1; f = file - 1;
	while ((r >= 0) && (r <= 7) && (f >= 0) && (f <= 7)) {
		SetBitTrue(mask, Square(r, f));
		if (CheckBit(blockers, Square(r, f))) break;
		r += 1; f -= 1;
	}

	return mask;
}

void GenerateMagicTables() {

	// 1. Populate rook magic results
	for (int sq = 0; sq < 64; sq++) {
		for (int i = 0; i < 4096; i++) {
			uint64_t occ = GenerateMagicOccupancy(i, RookMasks[sq]);
			int index = (occ * RookMagicNumbers[sq]) >> (64 - RookRelevantBits[sq]);
			RookAttacks[sq][index] = DynamicRookAttacks(sq, occ);
		}
	}

	// 2. Populate bishop magic results
	for (int sq = 0; sq < 64; sq++) {
		for (int i = 0; i < 512; i++) {
			uint64_t occ = GenerateMagicOccupancy(i, BishopMasks[sq]);
			int index = (occ * BishopMagicNumbers[sq]) >> (64 - BishopRelevantBits[sq]);
			BishopAttacks[sq][index] = DynamicBishopAttacks(sq, occ);
		}
	}
}

// Magic number generation (not actively needed) --------------------------------------------------

/*

void DebugBitboard(const uint64_t bits) {
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

void GenerateMagicNumbers() {
	cout << "Generating magic numbers..." << endl;

	// 1. Generate rook mask
	for (int i = 0; i < 64; i++) {
		uint64_t mask = 0;
		int file = GetSquareFile(i);
		int rank = GetSquareRank(i);

		for (int r = rank + 1; r < 7; r++) SetBitTrue(mask, Square(r, file));
		for (int r = rank - 1; r > 0; r--) SetBitTrue(mask, Square(r, file));
		for (int f = file + 1; f < 7; f++) SetBitTrue(mask, Square(rank, f));
		for (int f = file - 1; f > 0; f--) SetBitTrue(mask, Square(rank, f));

		RookMasks[i] = mask;
		RookRelevantBits[i] = Popcount(mask);
	}

	// 2. Generate bishop mask
	for (int i = 0; i < 64; i++) {
		uint64_t mask = 0;
		int file = GetSquareFile(i);
		int rank = GetSquareRank(i);

		// Ray towards bottom left
		int r = rank - 1; int f = file - 1;
		while ((r > 0) && (r < 7) && (f > 0) && (f < 7)) {
			SetBitTrue(mask, Square(r, f));
			r -= 1; f -= 1;
		}
		// Ray towards bottom right
		r = rank - 1; f = file + 1;
		while ((r > 0) && (r < 7) && (f > 0) && (f < 7)) {
			SetBitTrue(mask, Square(r, f));
			r -= 1; f += 1;
		}
		// Ray towards top right
		r = rank + 1; f = file + 1;
		while ((r > 0) && (r < 7) && (f > 0) && (f < 7)) {
			SetBitTrue(mask, Square(r, f));
			r += 1; f += 1;
		}
		// Ray towards top left
		r = rank + 1; f = file - 1;
		while ((r > 0) && (r < 7) && (f > 0) && (f < 7)) {
			SetBitTrue(mask, Square(r, f));
			r += 1; f -= 1;
		}

		BishopMasks[i] = mask;
		BishopRelevantBits[i] = Popcount(mask);
	}

	// 3. Set up random number generation and other variables
	std::random_device rd;
	std::mt19937_64 random(rd());
	std::uniform_int_distribution<unsigned long long> distr;
	std::vector<std::array<uint64_t, 4096>> RookMagicOccupancies;
	RookMagicOccupancies.reserve(64);
	std::vector<std::array<uint64_t, 512>> BishopMagicOccupancies;
	BishopMagicOccupancies.reserve(64);
	for (int i = 0; i < 64; i++) {
		RookMagicOccupancies.push_back(std::array<uint64_t, 4096>());
		BishopMagicOccupancies.push_back(std::array<uint64_t, 512>());
	}

	// 4. Generate rook magics
	for (int sq = 0; sq < 64; sq++) {
		int cases = 1 << RookRelevantBits[sq];
		for (int i = 0; i < cases; i++) {
			RookMagicOccupancies[sq][i] = GenerateMagicOccupancy(i, RookMasks[sq]);
			RookAttacks[sq][i] = DynamicRookAttacks(sq, RookMagicOccupancies[sq][i]);
		}

		while (true) {
			uint64_t candidate = distr(random) & distr(random) & distr(random);
			if (Popcount((candidate * RookMasks[sq]) & 0xFF00000000000000) < 6) continue;
			std::array<uint64_t, 4096> usedAttacks;
			for (int i = 0; i < 4096; i++) usedAttacks[i] = 0;

			bool failed = false;
			for (int i = 0; i < cases; i++) {
				int magicIndex = static_cast<int>((RookMagicOccupancies[sq][i] * candidate) >> (64 - RookRelevantBits[sq]));
				if ((usedAttacks[magicIndex] != 0) && (usedAttacks[magicIndex] != RookAttacks[sq][i])) {
					failed = true;
					break;
				}
				usedAttacks[magicIndex] = RookAttacks[sq][i];
			}
			if (failed) continue;

			RookMagicNumbers[sq] = candidate;
			break;
		}

	}

	// 5. Generate bishop magics
	for (int sq = 0; sq < 64; sq++) {
		int cases = 1 << BishopRelevantBits[sq];
		for (int i = 0; i < cases; i++) {
			BishopMagicOccupancies[sq][i] = GenerateMagicOccupancy(i, BishopMasks[sq]);
			BishopAttacks[sq][i] = DynamicBishopAttacks(sq, BishopMagicOccupancies[sq][i]);
		}

		while (true) {
			uint64_t candidate = distr(random) & distr(random) & distr(random);
			if (Popcount((candidate * BishopMasks[sq]) & 0xFF00000000000000) < 6) continue;
			std::array<uint64_t, 512> usedAttacks;
			for (int i = 0; i < usedAttacks.size(); i++) usedAttacks[i] = 0;

			bool failed = false;
			for (int i = 0; i < cases; i++) {
				int magicIndex = static_cast<int>((BishopMagicOccupancies[sq][i] * candidate) >> (64 - BishopRelevantBits[sq]));
				if ((usedAttacks[magicIndex] != 0) && (usedAttacks[magicIndex] != BishopAttacks[sq][i])) {
					failed = true;
					break;
				}
				usedAttacks[magicIndex] = BishopAttacks[sq][i];
			}
			if (failed) continue;

			BishopMagicNumbers[sq] = candidate;
			break;
		}

	}
}
*/