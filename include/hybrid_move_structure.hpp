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
    HybridMoveStructure(ifstream &bwt, u_int64_t text_length) {
        bwt.clear();
        bwt.seekg(0);

        rows = vector<Row>();
        vector<vector<u_int64_t>> L_block_indices =
            vector<vector<u_int64_t>>(kALPHABET_SIZE);

        uint last_c;
        int c_in;
        u_int64_t run = 0;
        u_int64_t idx = 0;
        r = 0;
        n = text_length;
        uint length = 0;
        std::vector<int> chars(kALPHABET_SIZE, 0);
        // to keep the count of each character, used in computing C
        std::vector<int> counts;
        // Temporarily build these to obtain B_FL
        sdsl::bit_vector B_F = sdsl::bit_vector(n, 0);
        sdsl::bit_vector B_L = sdsl::bit_vector(n, 0);
        sdsl::sd_vector B_F_sparse;
        sdsl::sd_vector B_L_sparse;
        // to hold the BWT offset of every run head
        std::vector<int> run_heads;
        // for computing the LF mapping for building the B_F vector
        std::vector<std::unique_ptr<sdsl::bit_vector>> occs;
        std::vector<std::unique_ptr<sdsl::rank_support_v<>>> occs_rank;

        char_to_index.resize(kALPHABET_SIZE);
        std::fill(char_to_index.begin(), char_to_index.end(), kALPHABET_SIZE);

        while ((c_in = bwt.get()) != EOF) {
            // increase count of the characters to find the characters that
            // exist in the BWT
            chars[c_in] += 1;

            uint c = static_cast<uint>(c_in);
            c = c <= kTerminator ? kTerminator : c;

            if (idx != 0 && c != last_c) {
                // save the run head character in H_L
                H_L.push_back(static_cast<char>(last_c));

                // TODO: this is a quick fix for the first bit of B_L not being
                // set
                if (idx - 1 == 0) {
                    B_L[0] = 1;
                    run_heads.push_back(idx - 1);
                }
                run_heads.push_back(idx);
                B_L[idx] = 1;

                rows.push_back({last_c, length, 0});
                L_block_indices[last_c].push_back(run++);
                // n += length;
                length = 0;
            }

            ++length;
            ++idx;
            last_c = c;
        }

        rows.push_back({last_c, length, 0});
        H_L.push_back(static_cast<char>(last_c));
        L_block_indices[last_c].push_back(run++);
        // n += length;

        r = rows.size();

        int sigma = 0;
        // Determining the number of B_x bit vectors
        for (int i = 0; i < kALPHABET_SIZE; i++) {
            if (chars[i] != 0) {
                char_to_index[i] = sigma;
                sigma += 1;
                counts.push_back(chars[i]);
                sdsl::bit_vector *new_b_vector = new sdsl::bit_vector(r, 0);
                B_x.emplace_back(
                    std::unique_ptr<sdsl::bit_vector>(new_b_vector));
                sdsl::bit_vector *new_occ_vector = new sdsl::bit_vector(n, 0);
                occs.emplace_back(
                    std::unique_ptr<sdsl::bit_vector>(new_occ_vector));
            }
        }

        // Building C
        int count = 0;
        for (int i = 0; i < counts.size(); i++) {
            C.push_back(count);
            count += counts[i];
        }

        // Building the B_x bit vectors
        for (int i = 0; i < r; i++) {
            (*B_x[char_to_index[H_L[i]]])[i] = 1;
        }
        // TODO: not sure if there should be anything else done to B_x before
        // this step
        //  create the rank objects for the B_x bit vectors
        for (auto &B : B_x) {
            B_x_ranks.emplace_back(std::unique_ptr<sdsl::rank_support_v<>>(
                new sdsl::rank_support_v<>(B.get())));
        }

        // Build B_F from B_L
        // TODO: I edited how the occs array is being filled
        int curr_idx = 0;
        for (int i = 0; i < r; i++) {
            char curr_char = H_L[i];
            int curr_length = rows[i].length;
            for (int j = curr_idx; j < curr_idx + curr_length; j++) {
                (*occs[char_to_index[curr_char]])[j] = 1;
            }
            curr_idx += curr_length;
        }

        // create the rank objects for the occurance bit vectors
        for (auto &occ : occs) {
            occs_rank.emplace_back(std::unique_ptr<sdsl::rank_support_v<>>(
                new sdsl::rank_support_v<>(occ.get())));
        }
        sdsl::rank_support_v<> rank_B_L = sdsl::rank_support_v(&B_L);
        for (int i = 0; i < r; i++) {
            int lf = 0;
            int alphabet_index = char_to_index[H_L[i]];
            lf += C[alphabet_index];
            // add the rank of the bwt_row
            auto &occ_rank = *occs_rank[alphabet_index];
            lf += static_cast<int>(occ_rank(run_heads[i]));
            B_F[lf] = 1;
        }
        // Create the sparse bit vectors for B_F and B_L
        B_F_sparse = sdsl::sd_vector<>(B_F);
        B_L_sparse = sdsl::sd_vector<>(B_L);

        // Build the B_FL bitvector
        //  initialize to 2*r bits
        sdsl::bit_vector B_FL_temp(2 * r);
        // track the index in B_FL
        size_t i_BFL = 0;
        // fill appropriate values in B_FL
        auto it_BF = B_F_sparse.begin();
        for (auto it_F = B_F_sparse.begin(), it_L = B_L_sparse.begin();
             it_F != B_F_sparse.end() && it_L != B_L_sparse.end();
             ++it_F, ++it_L, ++it_BF) {
            if (*it_F == 1 && *it_L == 1) {
                B_FL_temp[i_BFL] = 0;
                i_BFL++;
                B_FL_temp[i_BFL] = 1;
                i_BFL++;
            } else if (*it_F == 1) {
                B_FL_temp[i_BFL] = 1;
                i_BFL++;
            } else if (*it_L == 1) {
                B_FL_temp[i_BFL] = 0;
                i_BFL++;
            }
        }
        // keep it as a normal bit vector this time
        B_FL = B_FL_temp;
        select_1_B_FL = sdsl::select_support_mcl<>(&B_FL);

        // print results
        cout << "B_FL: " << endl;
        for (size_t i = 0; i < B_FL.size(); ++i) {
            cout << B_FL[i];
        }
        cout << endl;

        computeTable(L_block_indices);
        cout << "Rows: " << endl;
        for (const auto &row : rows) {
            cout << "Head: " << char(row.head) << ", Length: " << row.length
                 << ", Offset: " << row.offset << endl;
        }
    }

    // TODO: the C_array used here is the wrong one
    u_int64_t computePointer(uint64_t index) {
        uint64_t pi;
        char run_head = this->H_L[index];
        cout << "run_head: " << run_head << endl;
        cout << "C: " << C[this->char_to_index[run_head]] << endl;
        cout << "Rem: "
             << (*this->B_x_ranks[this->char_to_index[run_head]])(index)
             << endl;
        pi = this->C[this->char_to_index[run_head]] +
             (*this->B_x_ranks[this->char_to_index[run_head]])(index)-1;
        u_int64_t pointer = this->select_1_B_FL(pi + 1) - pi - 1;
        return pointer;
    }

    // TODO: check for off-by-one errors
    Position LF(Position pos) {
        size_t run_F, offset_F;
        size_t run_L = pos.run;
        size_t offset_L = pos.offset;
        size_t pointer = computePointer(run_L);
        size_t next_Pointer = computePointer(run_L + 1);
        run_F = pointer;
        offset_F = 0;
        while (offset_F <= offset_L) {
            if (pointer + offset_F == next_Pointer) {
                run_L += 1;
                pointer = next_Pointer;
                next_Pointer = computePointer(run_L + 1);
                run_F = pointer;
            }
            offset_F += 1;
        }
        return Position{run_F, offset_F};
    }

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

    std::vector<int> C;
    std::vector<char> H_L;
    std::vector<std::unique_ptr<sdsl::bit_vector>> B_x;

    // Rank data structure for B_x bit vectors
    std::vector<std::unique_ptr<sdsl::rank_support_v<>>> B_x_ranks;

    std::vector<int> char_to_index;
    sdsl::select_support_mcl<> select_1_B_FL;

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
