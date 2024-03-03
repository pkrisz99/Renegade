# This utility generates specific arrays and bitboards for little-endian rank-file encoded engines

class GenerateBoardConstants:

    # Internal constants

    ARRAY = False
    BITBOARD = True

    WHITE_PAWN_ATTACKS = 0
    BLACK_PAWN_ATTACKS = 1
    KNIGHT_MOVES = 2
    KING_MOVES = 6
    WHITE_PASSED_PAWN_MASK = 7
    WHITE_PASSED_PAWN_FILTER = 8
    BLACK_PASSED_PAWN_MASK = 9
    BLACK_PASSED_PAWN_FILTER = 10
    SQUARE_IDENTITY = 11
    KING_AREA = 12
    ISOLATED_PAWN_MASK = 13
    WHITE_BACKWARD_PAWN_MASK = 14
    BLACK_BACKWARD_PAWN_MASK = 15

    # Misc methods:

    @staticmethod
    def is_square_okay(sq: tuple[int, int]) -> bool:
        return (0 <= sq[0] <= 7) and (0 <= sq[1] <= 7)

    @staticmethod
    def convert_to_int(f: int, r: int) -> int:
        return r * 8 + f

    def filter_squares(self, unfiltered: list[tuple[int, int]]) -> list[tuple[int, int]]:
        filtered = []
        for sq in unfiltered:
            if self.is_square_okay(sq):
                filtered.append(sq)
        return filtered

    # Field generation:

    def get_white_pawn_attacks(self, f: int, r: int) -> list[tuple[int, int]]:
        if r == 7:
            return []
        l = [(f - 1, r + 1), (f + 1, r + 1)]
        return self.filter_squares(l)

    def get_black_pawn_attacks(self, f: int, r: int) -> list[tuple[int, int]]:
        if r == 0:
            return []
        l = [(f - 1, r - 1), (f + 1, r - 1)]
        return self.filter_squares(l)

    def get_knight_moves(self, f: int, r: int) -> list[tuple[int, int]]:
        l = [(f + 2, r + 1), (f + 2, r - 1), (f + 1, r + 2), (f + 1, r - 2), (f - 2, r + 1), (f - 2, r - 1),
             (f - 1, r + 2), (f - 1, r - 2)]
        return self.filter_squares(l)

    def get_king_moves(self, f: int, r: int) -> list[tuple[int, int]]:
        l = [(f + 1, r + 1), (f + 1, r), (f + 1, r - 1), (f, r + 1), (f, r - 1), (f - 1, r + 1), (f - 1, r),
             (f - 1, r - 1)]
        return self.filter_squares(l)

    def get_white_passed_pawn_mask(self, f: int, r: int) -> list[tuple[int, int]]:
        l = []
        r += 1
        while r < 8:
            l.append((f + 1, r))
            l.append((f, r))
            l.append((f - 1, r))
            r += 1
        return self.filter_squares(l)

    def get_white_passed_pawn_filter(self, f: int, r: int) -> list[tuple[int, int]]:
        l = []
        r += 1
        while r < 8:
            l.append((f, r))
            r += 1
        return self.filter_squares(l)

    def get_black_passed_pawn_mask(self, f: int, r: int) -> list[tuple[int, int]]:
        l = []
        r -= 1
        while r >= 0:
            l.append((f + 1, r))
            l.append((f, r))
            l.append((f - 1, r))
            r -= 1
        return self.filter_squares(l)

    def get_black_passed_pawn_filter(self, f: int, r: int) -> list[tuple[int, int]]:
        l = []
        r -= 1
        while r >= 0:
            l.append((f, r))
            r -= 1
        return self.filter_squares(l)

    @staticmethod
    def get_square_identity(f: int, r: int) -> list[tuple[int, int]]:
        return [(f, r)]

    def get_king_area(self, f: int, r: int) -> list[tuple[int, int]]:
        l = [(f + 1, r + 1), (f + 1, r), (f + 1, r - 1), (f, r + 1), (f, r), (f, r - 1), (f - 1, r + 1), (f - 1, r),
             (f - 1, r - 1)]
        return self.filter_squares(l)

    def get_isolated_pawn_mask(self, f: int, r: int) -> list[tuple[int, int]]:
        l = [(f - 1, 0), (f - 1, 1), (f - 1, 2), (f - 1, 3), (f - 1, 4), (f - 1, 5), (f - 1, 6), (f - 1, 7),
             (f + 1, 0), (f + 1, 1), (f + 1, 2), (f + 1, 3), (f + 1, 4), (f + 1, 5), (f + 1, 6), (f + 1, 7)]
        return self.filter_squares(l)

    def get_white_backward_pawn_mask(self, f: int, r: int) -> list[tuple[int, int]]:
        l = []
        i = r
        while i >= 0:
            l.append((f + 1, i))
            l.append((f - 1, i))
            i -= 1
        return self.filter_squares(l)

    def get_black_backward_pawn_mask(self, f: int, r: int) -> list[tuple[int, int]]:
        l = []
        i = r
        while i <= 7:
            l.append((f + 1, i))
            l.append((f - 1, i))
            i += 1
        return self.filter_squares(l)

    # Methods to call:

    def generate_fields(self, output_type, feature: int):
        move_list = []
        function_call = None

        match feature:
            case self.WHITE_PAWN_ATTACKS:
                function_call = self.get_white_pawn_attacks
            case self.BLACK_PAWN_ATTACKS:
                function_call = self.get_black_pawn_attacks
            case self.KNIGHT_MOVES:
                function_call = self.get_knight_moves
            case self.KING_MOVES:
                function_call = self.get_king_moves
            case self.WHITE_PASSED_PAWN_MASK:
                function_call = self.get_white_passed_pawn_mask
            case self.WHITE_PASSED_PAWN_FILTER:
                function_call = self.get_white_passed_pawn_filter
            case self.BLACK_PASSED_PAWN_MASK:
                function_call = self.get_black_passed_pawn_mask
            case self.BLACK_PASSED_PAWN_FILTER:
                function_call = self.get_black_passed_pawn_filter
            case self.SQUARE_IDENTITY:
                function_call = self.get_square_identity
            case self.KING_AREA:
                function_call = self.get_king_area
            case self.ISOLATED_PAWN_MASK:
                function_call = self.get_isolated_pawn_mask
            case self.WHITE_BACKWARD_PAWN_MASK:
                function_call = self.get_white_backward_pawn_mask
            case self.BLACK_BACKWARD_PAWN_MASK:
                function_call = self.get_black_backward_pawn_mask

        for rank in range(0, 8):
            for file in range(0, 8):

                squares = function_call(file, rank)
                nums = []
                for sq in squares:
                    nums.append(self.convert_to_int(sq[0], sq[1]))
                nums = sorted(nums)
                move_list.append(nums)

        if output_type == self.ARRAY:
            return self.create_array_string(move_list)
        elif output_type == self.BITBOARD:
            return self.create_bitboard_string(move_list)
        return move_list

    def generate_array(self, feature: int) -> str:
        return self.generate_fields(self.ARRAY, feature)

    def generate_bitboards(self, feature: int) -> str:
        return self.generate_fields(self.BITBOARD, feature)

    @staticmethod
    def create_array_string(move_list: list[list[int]]) -> str:
        string = "{"
        i = 0
        for sq in move_list:
            string += "{"
            j = 0
            for m in sq:
                string += "%d" % m
                j += 1
                if j != len(sq):
                    string += ","
            string += "}"
            if i != 63:
                string += ", "
            # if i % 8 == 7: string += "\n"
            i += 1
        string += "};"
        return string

    @staticmethod
    def create_bitboard_string(move_list: list[list[int]]) -> str:

        def uint64ify(array: list[int]) -> int:
            result = 0
            for n in array:
                result += 2 ** n
            return result

        string = "{"
        i = 0
        for sq in move_list:
            string += "{0:#0{1}x}".format(uint64ify(sq), 18)
            if i != 63:
                string += ", "
            if i % 8 == 7:
                string += "\n"
            i += 1
        string += "};"
        return string


if __name__ == '__main__':
    g = GenerateBoardConstants()
    print("\nconstexpr std::vector<std::vector<uint_8>> KnightMoves =", g.generate_array(g.KNIGHT_MOVES))
    print("\nconstexpr std::vector<std::vector<uint_8>> KingMoves =", g.generate_array(g.KING_MOVES))
    print()
    print("\nconstexpr uint64_t WhitePawnAttacks[] =", g.generate_bitboards(g.WHITE_PAWN_ATTACKS))
    print("\nconstexpr uint64_t BlackPawnAttacks[] =", g.generate_bitboards(g.BLACK_PAWN_ATTACKS))
    print("\nconstexpr uint64_t KnightAttacks[] =", g.generate_bitboards(g.KNIGHT_MOVES))
    print("\nconstexpr uint64_t KingAttacks[] =", g.generate_bitboards(g.KING_MOVES))
    print("\nconstexpr uint64_t WhitePassedPawnMask[] =", g.generate_bitboards(g.WHITE_PASSED_PAWN_MASK))
    print("\nconstexpr uint64_t WhitePassedPawnFilter[] =", g.generate_bitboards(g.WHITE_PASSED_PAWN_FILTER))
    print("\nconstexpr uint64_t BlackPassedPawnMask[] =", g.generate_bitboards(g.BLACK_PASSED_PAWN_MASK))
    print("\nconstexpr uint64_t BlackPassedPawnFilter[] =", g.generate_bitboards(g.BLACK_PASSED_PAWN_FILTER))
    print("\nconstexpr uint64_t SquareBits[] =", g.generate_bitboards(g.SQUARE_IDENTITY))
    print("\nconstexpr uint64_t KingArea[] =", g.generate_bitboards(g.KING_AREA))
    print("\nconstexpr uint64_t IsolatedPawnMask[] =", g.generate_bitboards(g.ISOLATED_PAWN_MASK))
    print("\nconstexpr uint64_t WhiteBackwardPawnMask[] =", g.generate_bitboards(g.WHITE_BACKWARD_PAWN_MASK))
    print("\nconstexpr uint64_t BlackBackwardPawnMask[] =", g.generate_bitboards(g.BLACK_BACKWARD_PAWN_MASK))
