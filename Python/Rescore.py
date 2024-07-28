import chess
import chess.syzygy

filename = "C:/Users/Krisz/Desktop/Renegade/x64/Release/net-6-text"
output = "C:/Users/Krisz/Desktop/Renegade/x64/Release/net-6-text-corrected2"
tb = "C:/Users/Krisz/Desktop/Renegade testing/syzygy-dl/"


def adjust_wdl():

    i = 0
    tablebase = chess.syzygy.open_tablebase(tb)
    tb_positions = 0
    bad = 0

    out_file = open(output, "a")
    print("Processed  Tablebase      Wrong")

    with open(filename) as file:
        for line in file:

            # Report
            if i % 100000 == 0 and i != 0:
                print("%9d  %9d  %9d  (%2.4f%%) " % (i, tb_positions, bad, bad / tb_positions * 100))
            i += 1

            # Parse line
            fen, evl, res = line.rstrip().split(" | ")
            piece_count = len([letter for letter in fen.split()[0] if letter.isalpha()])
            corrected = res

            # For tablebase positions
            if piece_count <= 6:
                board = chess.Board(fen)

                tb_positions += 1
                try:
                    tb_result = tablebase.probe_wdl(board)
                except KeyError:
                    tb_result = -1

                if board.turn == chess.WHITE:
                    if tb_result == -2 and res != "0.0":
                        corrected = "0.0"
                        bad += 1
                    elif tb_result == 0 and res != "0.5":
                        corrected = "0.5"
                        bad += 1
                    elif tb_result == 2 and res != "1.0":
                        corrected = "1.0"
                        bad += 1
                else:
                    if tb_result == -2 and res != "1.0":
                        corrected = "1.0"
                        bad += 1
                    elif tb_result == 0 and res != "0.5":
                        corrected = "0.5"
                        bad += 1
                    elif tb_result == 2 and res != "0.0":
                        corrected = "0.0"
                        bad += 1
                # Neglects cursed wins / blessed losses

            # Write to file
            out_file.write(f"{fen} | {evl} | {corrected}\n")

    tablebase.close()
    out_file.close()


adjust_wdl()