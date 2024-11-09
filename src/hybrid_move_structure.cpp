#include "hybrid_move_structure.hpp"
#include <cstdint>
#include <sdsl/sd_vector.hpp>
#include <vector>

class Row {
    unsigned int head : 3;
    unsigned int length : 10;
    unsigned int offset : 10;
};

class HybridMoveStructure {
  public:
    uint64_t n;
    uint64_t r;
    std::vector<Row> rows;
    sdsl::bit_vector B_FL;

    // TODO: load move structure from binary file
    HybridMoveStructure(std::string moveStructureFilename) {
        return null;
    }

    // TODO: Calculate the pointer field given an index
    uint64_t pointer(uint64_t index) {

        return 0;
    }

    // TODO: Save move structure to a binary file
    int storeMoveStructure(std::string filename) {
        return 0;
    }
};
