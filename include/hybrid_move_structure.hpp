#include <mach/notify.h>
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
#define kLength_Bytes 4
#define kOffset_Bytes 2
#define kChar_Bytes 3
#define BYTES_TO_BITS(bytes) ((bytes) * 8)

typedef unsigned int uint;

using namespace std;

class Row {
  public:
    unsigned int head : BYTES_TO_BITS(kChar_Bytes);
    unsigned int length : BYTES_TO_BITS(kLength_Bytes);
    unsigned int offset : BYTES_TO_BITS(kOffset_Bytes);

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
    /**
     * @brief Constructs Hybrid Move Structure given the BWT and its length
     */
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

        // stores the frequency of each character occurence
        std::vector<int> chars(kALPHABET_SIZE, 0);

        // stores the frequency of each run head occurence
        std::vector<int> chars_runs(kALPHABET_SIZE, 0);

        // to keep the count of each character, used in computing C
        std::vector<int> counts;

        // to keep the count of how many runs of each character exist
        std::vector<int> counts_runs;

        // to help B_L and B_F
        std::vector<u_int64_t> C;

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
                // increment the counter for the number of runs with the current
                // character
                chars_runs[c_in] += 1;

                // save the run head character in H_L
                H_L.push_back(static_cast<char>(last_c));

                if (rows.size() == 0) {
                    B_L[0] = 1;
                    run_heads.push_back(0);
                    chars_runs[last_c] += 1;
                }

                run_heads.push_back(idx);
                B_L[idx] = 1;

                rows.push_back({last_c, length, 0});
                L_block_indices[last_c].push_back(run++);
                length = 0;
            }

            ++length;
            ++idx;
            last_c = c;
        }

        rows.push_back({last_c, length, 0});
        H_L.push_back(static_cast<char>(last_c));
        L_block_indices[last_c].push_back(run++);

        r = rows.size();
        int sigma = 0;

        // determining the number of B_x bit vectors
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
                if (chars_runs[i] != 0) {
                    counts_runs.push_back(chars_runs[i]);
                }
            }
        }

        // building C and C_H vectors
        int count = 0;
        int count_runs = 0;
        for (int i = 0; i < counts.size(); i++) {
            C.push_back(count);
            count += counts[i];

            C_H.push_back(count_runs);
            count_runs += counts_runs[i];
        }

        // building the B_x bit vectors
        for (int i = 0; i < r; i++) {
            (*B_x[char_to_index[H_L[i]]])[i] = 1;
        }

        // create the rank objects for the B_x bit vectors
        for (auto &B : B_x) {
            B_x_ranks.emplace_back(std::unique_ptr<sdsl::rank_support_v<>>(
                new sdsl::rank_support_v<>(B.get())));
        }

        // build B_F from B_L
        // NOTE: @NourA edited how the occs array is being filled compared to
        // the one by done @MohsenZakeri
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

        // create the sparse bit vectors for B_F and B_L
        B_F_sparse = sdsl::sd_vector<>(B_F);
        B_L_sparse = sdsl::sd_vector<>(B_L);

        // build the B_FL bitvector
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

        B_FL = B_FL_temp;
        select_1_B_FL = sdsl::select_support_mcl<>(&B_FL);

        computeTable(L_block_indices);
        cout << "Rows: " << endl;
        for (const auto &row : rows) {
            cout << "Head: " << char(row.head) << ", Length: " << row.length
                 << ", Offset: " << row.offset << endl;
        }
    }

    /**
     * @brief Constructs Hybrid Move Structure from a file with the size of the 
     * BWT on the first line , the character and
     * length of each run of the BWT on the following lines.
     */
    HybridMoveStructure(ifstream &heads) {
        std::cout << "Reading heads" << std::endl;
        heads.clear();
        heads.seekg(0);

        heads >> n;
        heads.get();

        rows = vector<Row>();
        vector<vector<u_int64_t>> L_block_indices =
            vector<vector<u_int64_t>>(kALPHABET_SIZE);

        uint last_c;
        int c_in;
        uint c_length;
        u_int64_t run = 0;
        u_int64_t idx = 0;
        r = 0;
        uint length = 0;

        // stores the frequency of each character occurence
        std::vector<int> chars(kALPHABET_SIZE, 0);

        // stores the frequency of each run head occurence
        std::vector<int> chars_runs(kALPHABET_SIZE, 0);

        // to keep the count of each character, used in computing C
        std::vector<int> counts;

        // to keep the count of how many runs of each character exist
        std::vector<int> counts_runs;

        // to help B_L and B_F
        std::vector<u_int64_t> C;

        // temporarily build these to obtain B_FL
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

        while ((c_in = heads.get()) != EOF) {
            // increase count of the characters to find the characters that
            // exist in the BWT
            heads.get();
            heads >> c_length;
            chars[c_in] += c_length;
            heads.get();

            uint c = static_cast<uint>(c_in);
            c = c <= kTerminator ? kTerminator : c;

            // increment the counter for the number of runs with the current
            // character
            chars_runs[c_in] += 1;

            // save the run head character in H_L
            H_L.push_back(static_cast<char>(c_in));
            run_heads.push_back(idx);
            B_L[idx] = 1;
            rows.push_back({c, c_length, 0});
            L_block_indices[c_in].push_back(run++);

            idx += c_length;
        }

        r = rows.size();
        int sigma = 0;

        cout << "r: " << r << endl;

        // determining the number of B_x bit vectors
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
                if (chars_runs[i] != 0) {
                    counts_runs.push_back(chars_runs[i]);
                }
            }
        }

        // building C and C_H vectors
        int count = 0;
        int count_runs = 0;
        for (int i = 0; i < counts.size(); i++) {
            C.push_back(count);
            count += counts[i];

            C_H.push_back(count_runs);
            count_runs += counts_runs[i];
        }

        // building the B_x bit vectors
        for (int i = 0; i < r; i++) {
            (*B_x[char_to_index[H_L[i]]])[i] = 1;
        }

        //  create the rank objects for the B_x bit vectors
        for (auto &B : B_x) {
            B_x_ranks.emplace_back(std::unique_ptr<sdsl::rank_support_v<>>(
                new sdsl::rank_support_v<>(B.get())));
        }

        // build B_F from B_L
        // NOTE: @NourA edited how the occs array is being filled compared to
        // the one by done @MohsenZakeri
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

        // create the sparse bit vectors for B_F and B_L
        B_F_sparse = sdsl::sd_vector<>(B_F);
        B_L_sparse = sdsl::sd_vector<>(B_L);

        // build the B_FL bitvector
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

        B_FL = B_FL_temp;
        for (size_t i = 0; i < B_FL.size(); ++i) {
            cout << B_FL[i];
        }
        cout << endl;
        select_1_B_FL = sdsl::select_support_mcl<>(&B_FL);

        computeTable(L_block_indices);
        cout << "Rows: " << endl;
        for (const auto &row : rows) {
            cout << "Head: " << char(row.head) << ", Length: " << row.length
                 << ", Offset: " << row.offset << endl;
        }
    }

    /**
     * @brief Instead of storing the pointer for each row, the Hybrid Move
     * Structure computes the pointer value instead, reducing the memory usage
     * of the Move Structure.
     */
    u_int64_t computePointer(uint64_t index) {
        uint64_t pi = computePI(index);
        u_int64_t pointer = this->select_1_B_FL(pi + 1) - pi - 1;
        return pointer;
    }

    /**
     * @brief Computes the LF mapping of the first index of a run in the L 
     * column and returns its run in the F column
     */
    u_int64_t computePI(uint64_t index) {
        uint64_t pi;
        char run_head = this->H_L[index];

        pi = this->C_H[this->char_to_index[run_head]] +
             (*this->B_x_ranks[this->char_to_index[run_head]])(index);

        return pi;
    }

    /**
     * @brief Computes the LF mapping of a given (run, offset) pair in the BWT
     */
    Position LF(Position pos) {
        Position next_pos = {computePointer(pos.run), pos.offset};
        return next_pos;
    }

    /**
     * @brief Searches for a the number of occurrences of a pattern in the BWT 
     * using the computePointer (and computePI) functions
     */
    int backwards_search(char *pattern) {
        size_t len = strlen(pattern);
        cout << "Pattern: " << pattern << endl;

        // initialize offset and run pointers
        uint32_t sr = 0;
        uint32_t er = rows.size() - 1;
        size_t si = 0;
        size_t ei = rows[er].length - 1;

        for (size_t i = 0; i < len; i++) {
            uint c = static_cast<int>(pattern[len - i - 1]);

            // find the next run range
            while (rows[sr].head != c) {
                sr += 1;
                si = 0;
            }
            while (rows[er].head != c) {
                er -= 1;
                ei = rows[er].length - 1;
            }

            // break if the range is invalid
            if (sr > er) {
                printf("returned: 0\n");
                return 0;
            }

            // map the current range to F and get that range's pointers in L
            uint8_t s_off = rows[sr].offset;
            uint8_t e_off = rows[er].offset;
            sr = computePointer(sr);
            er = computePointer(er);
            ei += e_off;
            si += s_off;

            // adjust for overflow in offset value
            while (si > rows[sr].length) {
                si -= rows[sr].length;
                sr += 1;
            }
            while (ei > rows[er].length) {
                ei -= rows[er].length;
                er += 1;
            }
        }

        // print result
        u_int64_t count =
            er == sr ? ei - si + 1 : (er - sr) + ei + (rows[sr].length - si);
        cout << "Count: " << count << endl;

        return count;
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
    std::vector<u_int64_t> C_H;
    std::vector<char> H_L;
    std::vector<std::unique_ptr<sdsl::bit_vector>> B_x;

    // rank data structure for B_x bit vectors
    std::vector<std::unique_ptr<sdsl::rank_support_v<>>> B_x_ranks;

    // to map a character to the index
    // #:0, $:1, A:2, C:3, G:4, T:5
    std::vector<int> char_to_index;

    // select data structure for B_FL bitvector
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

#endif // !__HYBRID_MOVE_STRUCTURE__endif // !__HYBRID_MOVE_STRUCTURE__
