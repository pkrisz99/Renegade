# This script generates 'position startpos moves ...' strings and FENs from a PGN file.
# It can be used for debugging purposes and requires the python-chess library.

import chess
import chess.pgn
import sys

if __name__ == '__main__':

    filename = "test.pgn"
    if len(sys.argv) > 1:
        filename = sys.argv[1]

    with open(filename) as pgn:
        game = chess.pgn.read_game(pgn)

    board = game.board()
    string = "position startpos moves "
    for move in game.mainline_moves():
        string += move.uci() + " "
        board.push(move)
        print(f"Move {board.fullmove_number}")
        print(string)
        print(board.fen(), "\n")
