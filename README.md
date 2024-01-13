<div align = "center"><h1>Renegade</h1></div>

**Renegade is a free and open source chess engine. ♞**  

It is written in C++ from scratch, and values readability and simplicity. As the engine communicates through the UCI protocol, it can be connected to almost all chess user interfaces.  

The engine is fairly strong, and regularly competes on Lichess over at https://lichess.org/@/RenegadeEngine, as well as in tournaments organized by the engine testing community.  

## Features
### Board representation
- Uses bitboards to represent the location of the pieces on the board, which allows for fast and efficient operations
- A redundant mailbox is used to query what piece is on a given square, and a board hash vector is kept around for repetition checking
- Move generation relies on a number of precalculated tables, including plain magic bitboards for sliding pieces

### Search
- The engine uses a fairly standard fail-soft alpha-beta pruning framework with iterative deepening and principal variation search
- A number of move ordering and pruning methods are implemented to make search more efficient (see `Search.cpp`)

### Evaluation
- Renegade makes use of modern NNUE (efficiently updatable neural network) technology for accurate position evaluation
- Its neural network was trained entirely on self-play data, amounting to over 640 million positions
- The network architecture is a 768->512x2->1 perspective net with SCReLU activation 

## Usage
Renegade - like most chess engines - is a command line application implementing the UCI protocol. It should be used alongside a graphical user interface, such as [Cute Chess](https://github.com/cutechess/cutechess).  

As one might expect, Renegade can also be interacted with using the terminal. Most UCI commands are supported including limiting search to a specific depth, time limit or node count.

Scores are scaled according to the estimated outcome of the game, an evaluation of 100 centipawns represents a 50% likelihood of winning.

If the engine doesn't receive the `uci` command, it defaults to pretty print outputs (if the compiler supports it):
```
Renegade chess engine 0.12.0 [Oct 12 2023 17:33:27]
go depth 7
  1/ 1         20      0ms   320knps  h= 0.0%  ->  +0.47  [g1f3]
  2/ 2         77      0ms   179knps  h= 0.0%  ->  +0.20  [g1f3 g8f6]
  3/ 3        241      0ms   340knps  h= 0.0%  ->  +0.42  [g1f3 g8f6 b1c3]
  4/ 6        617      1ms   559knps  h= 0.0%  ->  +0.20  [g1f3 g8f6 b1c3 b8c6]
  5/ 6       1480      1ms   825knps  h= 0.0%  ->  +0.32  [g1f3 g8f6 b1c3 b8c6 e2e3]
  6/ 9       3242      3ms   989knps  h= 0.0%  ->  +0.20  [g1f3 g8f6 b1c3 b8c6 e2e3 (+1)]
  7/12       6473      5ms  1175knps  h= 0.0%  ->  +0.18  [g1f3 g8f6 b1c3 d7d5 e2e3 (+2)]
bestmove g1f3
```

Some useful custom commands are also implemented, such as `eval`, `draw` and `fen`. Typing `plain` will disable the colorful output, which is helpful if your terminal doesn't support it.

## Compilation

The recommended compiler is Clang 16. It should be noted that the engine makes use of modern hardware instructions for the best possible performance.  

Windows executables can be found for each release.

## Acknowledgments
Getting this far would not have been possible without the members of the [Engine Programming Discord](https://github.com/EngineProgramming/engine-list), and the decades of prior research done into chess programming.  

In particular, Renegade took many ideas from [Akimbo](https://github.com/jw1912/akimbo), [Ethereal](https://github.com/AndyGrant/Ethereal), [Stockfish](https://github.com/official-stockfish/Stockfish), [Stormphrax](https://github.com/Ciekce/Stormphrax), [Viridithas](https://github.com/cosmobobak/viridithas) and [Wahoo](https://github.com/spamdrew128/Wahoo).  

The neural networks were trained with [bullet](https://github.com/jw1912/bullet), and win-draw-loss models have been calculated using [Stockfish's WDL tool](https://github.com/official-stockfish/WDL_model).  

Additionally, I would also like to thank everyone who spent the time trying out and testing the engine, which provides valuable feedback for me.  