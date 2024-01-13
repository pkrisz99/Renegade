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
Renegade chess engine 1.0.0 [Jan 13 2024 20:13:23]
go depth 7
  1/ 1         20      0ms   189knps  h= 0.0%  ->  +0.23  31-48-22  [d2d4]
  2/ 4         74      0ms   172knps  h= 0.0%  ->  +0.34  33-47-20  [d2d4 d7d5]
  3/ 4        167      0ms   249knps  h= 0.0%  ->  +0.22  30-48-22  [d2d4 d7d5 g1f3]
  4/ 5        327      0ms   391knps  h= 0.0%  ->  +0.27  32-48-21  [d2d4 d7d5 g1f3 g8f6]
  5/10       1024      1ms   800knps  h= 0.0%  ->  +0.32  33-47-20  [e2e4 e7e5 d2d4 e5d4 d1d4]
  6/11       3200      2ms  1336knps  h= 0.0%  ->  +0.44  36-46-18  [e2e4 c7c5 g1f3 e7e6 f1e2 (+1)]
  7/12       4756      3ms  1448knps  h= 0.0%  ->  +0.32  33-47-20  [e2e4 e7e6 d2d4 d7d5 e4d5 (+2)]
bestmove e2e4
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