#include "Magics.h"

// Largely based on https://github.com/maksimKorzh/chess_programming/blob/master/src/magics/magics.c
// and indirectly on https://www.chessprogramming.org/Looking_for_Magics#Feeding_in_Randoms

// Retrieving attack bitboards --------------------------------------------------------------------

uint64_t GetRookAttacks(const uint8_t square, const uint64_t occupancy) {
	const int index = static_cast<int>(((occupancy & RookMasks[square]) * RookMagicNumbers[square]) >> (64 - RookRelevantBits[square]));
	return RookAttacks[square][index];
}

uint64_t GetBishopAttacks(const uint8_t square, const uint64_t occupancy) {
	const int index = static_cast<int>(((occupancy & BishopMasks[square]) * BishopMagicNumbers[square]) >> (64 - BishopRelevantBits[square]));
	return BishopAttacks[square][index];
}

uint64_t GetQueenAttacks(const uint8_t square, const uint64_t occupancy) {
	const int rookIndex = static_cast<int>(((occupancy & RookMasks[square]) * RookMagicNumbers[square]) >> (64 - RookRelevantBits[square]));
	const int bishopIndex = static_cast<int>(((occupancy & BishopMasks[square]) * BishopMagicNumbers[square]) >> (64 - BishopRelevantBits[square]));
	return BishopAttacks[square][bishopIndex] | RookAttacks[square][rookIndex];
}

uint64_t GetShortConnectingRay(const uint8_t from, const uint8_t to) {
	return ShortConnectingRays[from][to];
}

uint64_t GetLongConnectingRay(const uint8_t from, const uint8_t to) {
	return LongConnectingRays[from][to];
}

// Things for lookup table population -------------------------------------------------------------

// Generate an occupancy bitboard where the encoded bits represent occupied bits in the mask
uint64_t GenerateMagicOccupancy(const int encoded, uint64_t mask) {
	uint64_t map = 0;
	const int popcount = Popcount(mask);
	int i = popcount - 1;
	while (Popcount(mask) != 0) {
		const uint8_t sq = 63 - Lzcount(mask);
		if (CheckBit(static_cast<uint64_t>(encoded), i)) SetBitTrue(map, sq);
		SetBitFalse(mask, sq);
		i -= 1;
	}
	return map;
}

// Dynamically generate rook attacks
uint64_t DynamicRookAttacks(const int square, const uint64_t blockers) {
	uint64_t mask = 0;
	const int file = GetSquareFile(square);
	const int rank = GetSquareRank(square);

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
	const int file = GetSquareFile(square);
	const int rank = GetSquareRank(square);

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
			const uint64_t occ = GenerateMagicOccupancy(i, RookMasks[sq]);
			const int index = static_cast<int>((occ * RookMagicNumbers[sq]) >> (64 - RookRelevantBits[sq]));
			RookAttacks[sq][index] = DynamicRookAttacks(sq, occ);
		}
	}

	// 2. Populate bishop magic results
	for (int sq = 0; sq < 64; sq++) {
		for (int i = 0; i < 512; i++) {
			const uint64_t occ = GenerateMagicOccupancy(i, BishopMasks[sq]);
			const int index = static_cast<int>((occ * BishopMagicNumbers[sq]) >> (64 - BishopRelevantBits[sq]));
			BishopAttacks[sq][index] = DynamicBishopAttacks(sq, occ);
		}
	}

	// 3. Generate short connecting rays
	// 	  note: the rays include the from and to squares
	//    (and ye, this is not magic, but for now it's an alright place for this)
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			ShortConnectingRays[i][j] = [&] {
				if (i == j) return SquareBit(i);
				if (GetSquareFile(i) == GetSquareFile(j) || GetSquareRank(i) == GetSquareRank(j)) {
					const uint64_t ray1 = GetRookAttacks(i, SquareBit(j));
					const uint64_t ray2 = GetRookAttacks(j, SquareBit(i));
					return (ray1 & ray2) | SquareBit(i) | SquareBit(j);
				}
				else if (std::abs(GetSquareFile(i) - GetSquareFile(j)) == std::abs(GetSquareRank(i) - GetSquareRank(j))) {
					const uint64_t ray1 = GetBishopAttacks(i, SquareBit(j));
					const uint64_t ray2 = GetBishopAttacks(j, SquareBit(i));
					const uint64_t ends = (ray1 & ray2) ? SquareBit(i) | SquareBit(j) : 0ull;
					return (ray1 & ray2) | ends;
				}
				return uint64_t{0};
			}();
		}
	}

	// 4. Generate long connecting rays, this not only returns the squares between, but also the full rank/row/diagonal
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			LongConnectingRays[i][j] = [&] {
				if (i == j) return SquareBit(i);
				if (GetSquareFile(i) == GetSquareFile(j) || GetSquareRank(i) == GetSquareRank(j)) {
					const uint64_t ray1 = GetRookAttacks(i, 0ull);
					const uint64_t ray2 = GetRookAttacks(j, 0ull);
					return (ray1 & ray2) | SquareBit(i) | SquareBit(j);
				}
				else if (std::abs(GetSquareFile(i) - GetSquareFile(j)) == std::abs(GetSquareRank(i) - GetSquareRank(j))) {
					const uint64_t ray1 = GetBishopAttacks(i, 0ull);
					const uint64_t ray2 = GetBishopAttacks(j, 0ull);
					const uint64_t ends = (ray1 & ray2) ? SquareBit(i) | SquareBit(j) : 0ull;
					return (ray1 & ray2) | ends;
				}
				return uint64_t{0};
			}();
		}
	}
}

// Magic number generation (not actively needed) --------------------------------------------------

/*
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