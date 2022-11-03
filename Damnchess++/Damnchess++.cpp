// Damnchess++.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "Board.h"
#include "Engine.h"
#include "Evaluation.h"

#include <iostream>
#include <tuple>

int main() {

    //Board board = Board("2r3k1/1q1nbppp/r3p3/3pP3/pPpP4/P1Q2N2/2RN1PPP/2R4K b - b3 0 23");
    Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //board.Draw();
    //board.Test();

    Engine engine = Engine();
    /*
    for (int i = 1; i < 8; i++) {
        engine.perft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", i);
    }*/

    cout << "Started." << endl;

    
    //Evaluation result = engine.Search(board, 3);
    //cout << result.score << " - " << result.move.ToString() << " (" << result.nodes << " nodes, " << result.nps << " nps)" << endl;
      
    //engine.Play();
    engine.Start();

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
