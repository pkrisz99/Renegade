#include "Magics.h"

// Magic number generator for sliding attack lookup
// Still needs to be cleaned up

// Largely based on https://github.com/maksimKorzh/chess_programming/blob/master/src/magics/magics.c
// and indirectly on https://www.chessprogramming.org/Looking_for_Magics#Feeding_in_Randoms

uint64_t RookAttacks(const int square, const uint64_t blockers) {
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

uint64_t BishopAttacks(const int square, const uint64_t blockers) {
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

void GenerateMagics() {
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

	// 4. Generate rook magics
	for (int sq = 0; sq < 64; sq++) {
		int cases = 1 << RookRelevantBits[sq];
		for (int i = 0; i < cases; i++) {
			RookMagics[sq].Occupancies[i] = GenerateMagicOccupancy(i, RookMasks[sq]);
			RookMagics[sq].Attacks[i] = RookAttacks(sq, RookMagics[sq].Occupancies[i]);
		}

		while (true) {
			uint64_t candidate = distr(random) & distr(random) & distr(random);
			if (Popcount((candidate * RookMasks[sq]) & 0xFF00000000000000) < 6) continue;
			std::array<uint64_t, 4096> usedAttacks;
			for (int i = 0; i < 4096; i++) usedAttacks[i] = 0;

			bool failed = false;
			for (int i = 0; i < cases; i++) {
				int magicIndex = static_cast<int>((RookMagics[sq].Occupancies[i] * candidate) >> (64 - RookRelevantBits[sq]));
				if ((usedAttacks[magicIndex] != 0) && (usedAttacks[magicIndex] != RookMagics[sq].Attacks[i])) {
					failed = true;
					break;
				}
				usedAttacks[magicIndex] = RookMagics[sq].Attacks[i];
			}
			if (failed) {
				continue;
			}

			RookMagicNumbers[sq] = candidate;
			break;
		}

	}

	// 5. Generate bishop magics
	for (int sq = 0; sq < 64; sq++) {
		int cases = 1 << BishopRelevantBits[sq];
		for (int i = 0; i < cases; i++) {
			BishopMagics[sq].Occupancies[i] = GenerateMagicOccupancy(i, BishopMasks[sq]);
			BishopMagics[sq].Attacks[i] = BishopAttacks(sq, BishopMagics[sq].Occupancies[i]);
		}

		while (true) {
			uint64_t candidate = distr(random) & distr(random) & distr(random);
			if (Popcount((candidate * BishopMasks[sq]) & 0xFF00000000000000) < 6) continue;
			std::array<uint64_t, 512> usedAttacks;
			for (int i = 0; i < usedAttacks.size(); i++) usedAttacks[i] = 0;

			bool failed = false;
			for (int i = 0; i < cases; i++) {
				int magicIndex = static_cast<int>((BishopMagics[sq].Occupancies[i] * candidate) >> (64 - BishopRelevantBits[sq]));
				if ((usedAttacks[magicIndex] != 0) && (usedAttacks[magicIndex] != BishopMagics[sq].Attacks[i])) {
					failed = true;
					break;
				}
				usedAttacks[magicIndex] = BishopMagics[sq].Attacks[i];
			}
			if (failed) continue;

			BishopMagicNumbers[sq] = candidate;
			break;
		}

	}

	for (int sq = 0; sq < 64; sq++) {
		for (int i = 0; i < 4096; i++) {
			uint64_t occ = GenerateMagicOccupancy(i, RookMasks[sq]);
			int index = (occ * RookMagicNumbers[sq]) >> (64 - RookRelevantBits[sq]);
			RookMagics[sq].Attacks[index] = RookAttacks(sq, occ);
		}
	}

	for (int sq = 0; sq < 64; sq++) {
		for (int i = 0; i < 512; i++) {
			uint64_t occ = GenerateMagicOccupancy(i, BishopMasks[sq]);
			int index = (occ * BishopMagicNumbers[sq]) >> (64 - BishopRelevantBits[sq]);
			BishopMagics[sq].Attacks[index] = BishopAttacks(sq, occ);
		}
	}
}

uint64_t GetRookAttacks(const int square, const uint64_t occupancy) {
	int index = static_cast<int>(((occupancy & RookMasks[square]) * RookMagicNumbers[square]) >> (64 - RookRelevantBits[square]));
	return RookMagics[square].Attacks[index];
}

uint64_t GetBishopAttacks(const int square, const uint64_t occupancy) {
	int index = static_cast<int>(((occupancy & BishopMasks[square]) * BishopMagicNumbers[square]) >> (64 - BishopRelevantBits[square]));
	return BishopMagics[square].Attacks[index];
}