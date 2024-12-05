#include "hybrid_move_structure.hpp"

int main(int argc, char *argv[]) {
    std::ifstream file(argv[1]);
    auto text_length = std::stoull(argv[2]);
    if (!file) {
        std::cerr << "Unable to open file: " << argv[1] << std::endl;
        return 1;
    }

    HybridMoveStructure hmv(file, text_length);

    // assert(hmv.computePointer(1) == 9);
    // assert(hmv.computePointer(0) == 1);
    // assert(hmv.computePointer(10) == 13);

    // Position pos = hmv.LF({2, 0});

    // assert(pos.run == 1);
    // assert(pos.offset == 0);

    // std::ifstream substrings_file("/Users/nourallam/Desktop/Hybrid-Move-Structure/src/substrings.txt");
    // if (!substrings_file) {
    //     std::cerr << "Unable to open file: substrings.txt" << std::endl;
    //     return 1;
    // }

    // std::string substring;
    // while (std::getline(substrings_file, substring)) {
    //     hmv.backwards_search((char*)substring.c_str());
    // }
    hmv.backwards_search(argv[3]);
}
