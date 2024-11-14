#ifndef __HYBRID_MOVE_STRUCTURE__

#include <fstream>
#include <sdsl/sd_vector.hpp>
#include <sys/_types/_u_int64_t.h>
#include <vector>

// LEAVE UNCHANGED
#define kALPHABET_SIZE 256
#define kRW_BYTES 5

// CONFIGURABLE
#define kTerminator 1
#define LENGTH_BYTES 4
#define OFFSET_BYTES 2
#define CHAR_BYTES 3
#define BYTES_TO_BITS(bytes) ((bytes) * 8)

typedef unsigned int uint;

using namespace std;

class Row {
  public:
    unsigned int head : BYTES_TO_BITS(CHAR_BYTES);
    unsigned int length : BYTES_TO_BITS(LENGTH_BYTES);
    unsigned int offset : BYTES_TO_BITS(OFFSET_BYTES);

    Row() {}
    Row(uint c, uint l, uint o) : head(c), length(l), offset(o) {}
} __attribute__((packed));

struct Position {
    u_int64_t run = 0;
    u_int64_t offset = 0;
};

class HybridMoveStructure {
  public:
    HybridMoveStructure() {}
    HybridMoveStructure(ifstream &bwt) {
        bwt.clear();
        bwt.seekg(0);

        rows = vector<Row>();
        vector<vector<size_t>> L_block_indices =
            vector<vector<size_t>>(kALPHABET_SIZE);

        uint last_c;
        int c_in;
        u_int64_t run = 0;
        u_int64_t idx = 0;
        r = 0;
        n = 0;
        uint length = 0;

        while ((c_in = bwt.get()) != EOF) {
            uint c = static_cast<uint>(c_in);
            c = c <= kTerminator ? kTerminator : c;

            if (idx != 0 && c != last_c) {
                rows.push_back({last_c, length, 0});
                L_block_indices[last_c].push_back(run++);
                n += length;
                length = 0;
            }

            ++length;
            ++idx;
            last_c = c;
        }

        rows.push_back({last_c, length, 0});
        L_block_indices[last_c].push_back(run++);
        n += length;

        r = rows.size();
    }

    u_int64_t computePointer(uint64_t index) {

        return 0;
    }

    Position LF(Position pos) {}

    const Row get(u_int64_t pos) {
        assert(pos < rows.size());
        return rows[pos];
    }

    const Row get(Position pos) {
        assert(pos.run < rows.size());
        return rows[pos.run];
    }

    u_int64_t size() {
        return n;
    }

    u_int64_t runs() {
        return r;
    }

  protected:
    u_int64_t n;
    u_int64_t r;
    vector<Row> rows;
    sdsl::bit_vector B_FL;

    void computeTable(vector<vector<u_int64_t>> L_block_indices) {
        u_int64_t curr_L_num = 0;
        u_int64_t L_seen = 0;
        u_int64_t F_seen = 0;
        for (size_t i = 0; i < L_block_indices.size(); i++) {
            for (size_t j = 0; j < L_block_indices[i].size(); j++) {
                u_int64_t pos = L_block_indices[i][j];

                rows[pos].offset = F_seen - L_seen;
                F_seen += get(pos).length;

                while (curr_L_num < r &&
                       F_seen >= L_seen + get(curr_L_num).length) {
                    L_seen += get(curr_L_num).length;
                    ++curr_L_num;
                }
            }
        }
    }
};

#endif // !__HYBRID_MOVE_STRUCTURE__
