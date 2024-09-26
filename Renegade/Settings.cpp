#include "Settings.h"

namespace Settings {
    int Hash = HashDefault;
    int Threads = ThreadsDefault;
    bool ShowWDL = ShowWDLDefault;
    bool UseUCI = false;
    bool Chess960 = Chess960Default;
}

namespace Tune {

    // Define values here:

    /*Tunable see_pawn = Tunable{"see_pawn", 50, 250, 100, 20};
    Tunable see_knight = Tunable{ "see_knight", 250, 500, 300, 20 };
    Tunable see_bishop = Tunable{ "see_bishop", 250, 500, 300, 20 };
    Tunable see_rook = Tunable{ "see_rook", 400, 850, 500, 40 };
    Tunable see_queen = Tunable{ "see_queen", 800, 1300, 1000, 40 };

    Tunable see_quiet = Tunable{ "see_quiet", 30, 80, 50, 10 };
    Tunable see_noisy = Tunable{ "see_noisy", 60, 130, 100, 10 };
    Tunable see_search_th = Tunable{ "see_search_th", 0, 100, 0, 20 };
    Tunable see_qsearch_th = Tunable{ "see_qsearch_th", 0, 100, 0, 20 };*/

    /*Tunable redhist_negscale = Tunable{"redhist_negscale", 2, 16, 4, 2};

    Tunable redhist_0 = Tunable{ "redhist_0", 600, 4000, 1800, 350 };
    Tunable redhist_1 = Tunable{ "redhist_1", 500, 3000, 1500, 300 };
    Tunable redhist_2 = Tunable{ "redhist_2", 400, 2500, 1200, 250 };
    Tunable redhist_3 = Tunable{ "redhist_3", 300, 1500, 900, 200 };
    Tunable redhist_4 = Tunable{ "redhist_4", 200, 1000, 600, 150 };

    Tunable redhist_reset = Tunable{ "redhist_reset", 1, 2000, 1, 500 };*/

    std::unordered_map<std::string_view, Tunable&> List = {
        /*
        {see_pawn.name, see_pawn},
        {see_knight.name, see_knight},
        {see_bishop.name, see_bishop},
        {see_rook.name, see_rook},
        {see_queen.name, see_queen},
        {see_quiet.name, see_quiet},
        {see_noisy.name, see_noisy},
        {see_search_th.name, see_search_th},
        {see_qsearch_th.name, see_qsearch_th},
        {redhist_negscale.name, redhist_negscale},
        {redhist_reset.name, redhist_reset},
        {redhist_0.name, redhist_0},
        {redhist_1.name, redhist_1},
        {redhist_2.name, redhist_2},
        {redhist_3.name, redhist_3},
        {redhist_4.name, redhist_4},*/
    };

    // Helper functions:

    void PrintOptions() {
        for (const auto& [name, param] : List) {
            cout << "option name " << param.name << " type spin default " << param.default_value << " min " << param.min << " max " << param.max << endl;
        }
    }

    void GenerateString() {
        cout << "Currently " << List.size() << " tunable values are plugged in:" << endl;
        for (const auto& [name, param] : List) {
            cout << param.name << ", ";
            cout << "int" << ", ";
            cout << param.default_value << ", ";
            cout << param.min << ", ";
            cout << param.max << ", ";
            cout << param.step << ", ";
            cout << "0.002 " << endl;
        }
    }

    bool Active() {
        return List.size() != 0;
    }

}