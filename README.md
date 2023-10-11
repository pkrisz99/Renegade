<div align = "center"><h1>Renegade</h1></div>

**Renegade is a simple free and open source chess engine. ♞**  

It is written in C++ from scratch, and values readability and simplicity. As the engine communicates through the UCI protocol, it can be connected to almost all chess user interfaces.  

The engine is moderately strong, and regularly competes on Lichess over at https://lichess.org/@/RenegadeEngine, as well as in tournaments organized by the engine testing community.  

## Features
### Board representation
- Uses bitboards to represent the location of the pieces on the board, which allows for fast and efficient operations
- A redundant mailbox is used to query what piece is on a given square, and a board hash vector is kept around for repetition checking
- Move generation relies on a number of precalculated tables, including plain magic bitboards for sliding pieces

### Search
- The engine uses a fairly standard fail-soft alpha-beta pruning framework with iterative deepening and principal variation search
- A number of move ordering and pruning methods are implemented to make search more efficient (see `Search.cpp`)

### Evaluation
- A hand-crafted evaluation function is used with an automated tuner setting its over 800 parameter pairs on 7 million positions
- The evaluation weights are stored as middlegame-endgame pairs, and a linear interpolation is used to get the exact value
- Evaluation terms include mobility, passed pawns, open files, bishop pairs, king safety, outposts, and many more
- The engine tries to avoid trading down to drawish endgames if it has a material advantage

## Usage
Renegade - like most chess engines - is a command line application implementing the UCI protocol. It should be used alongside a graphical user interface, such as [Cute Chess](https://github.com/cutechess/cutechess).

The engine is generally stable and shouldn't make invalid moves or forfeit on time. The biggest issue is the inability to listen for `stop` commands or responding to `isready` while busy. These can also cause problems in Banksia in case of longer time controls.  

As one might expect, Renegade can also be interacted with using the terminal. Most UCI commands are supported including limiting search to a specific depth, time limit or node count. Hash size can be configured, and it automatically loads any `book.bin` opening book it finds in the same folder, but only uses it when `OwnBook` is set to `true`.  

If the engine doesn't receive the `uci` command, it defaults to pretty print outputs:
```
Renegade chess engine 0.11.0 [May 29 2023 12:31:30]
go depth 7
  1/ 1         20      0ms   326knps  h= 0.0%  ->  +0.46  [g1f3]
  2/ 2         93      0ms   154knps  h= 0.0%  ->  +0.18  [g1f3 g8f6]
  3/ 3        272      0ms   281knps  h= 0.0%  ->  +0.39  [g1f3 g8f6 b1c3]
  4/ 6       1059      1ms   530knps  h= 0.0%  ->  +0.18  [g1f3 g8f6 b1c3 b8c6]
  5/ 7       1974      3ms   614knps  h= 0.0%  ->  +0.33  [g1f3 g8f6 b1c3 b8c6 d2d4]
  6/12       5190      6ms   751knps  h= 0.0%  ->  +0.18  [g1f3 g8f6 b1c3 b8c6 d2d4 (+1)]
  7/12      15014     14ms  1036knps  h= 0.0%  ->  +0.14  [g1f3 g8f6 b1c3 b8c6 d2d4 (+2)]
bestmove g1f3
```

Some useful custom commands are also implemented, such as `eval`, `draw` and `fen`. Typing `plain` will disable the colorful output, which is helpful if your terminal doesn't support it.

## Compilation

For the best performance, it is best to compile using the makefile. The recommended compiler is Clang 16.  

In case of Windows, the project can also be compiled using Visual Studio 2019 with C++20 features enabled. The resulting executable should be around 10% slower.  

Windows executables can be found for each release.

## Acknowledgments
Getting this far would not have been possible without the contributors of the [Chess Programming Wiki](https://www.chessprogramming.org/Main_Page) and the members of the [Engine Programming Discord](https://github.com/EngineProgramming/engine-list), and I'm deeply grateful for [Maksim Korzh](https://youtube.com/playlist?list=PLmN0neTso3Jxh8ZIylk74JpwfiWNI76Cs) and [Bluefever Software](https://youtube.com/playlist?list=PLZ1QII7yudbc-Ky058TEaOstZHVbT-2hg) for making a video series on this subject.  

Additionally, I would also like to thank everyone who spent the time trying out and testing the engine, which provides valuable feedback for me.  