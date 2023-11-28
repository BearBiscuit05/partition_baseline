#ifndef HDRFPP__GREEDY_HPP_
#define HDRFPP__GREEDY_HPP_

#include "globals.hpp"
#include "bitset"
#include <fstream>
#include <iostream>
#include "cuckoo_map_pow2.hpp"

#define RPC_MAX_N_PROCS 32

typedef fixed_dense_bitset<RPC_MAX_N_PROCS> bin_counts_type; 
typedef cuckoo_map_pow2<int, bin_counts_type,3,uint32_t> degree_hash_table_type;



class Greedy
{
private:
    Globals& globals;
    uint64_t min_load = UINT64_MAX;
    uint64_t max_load = 0;
    double seed;
    int shrink{};

#ifdef STATS
    std::vector<uint64_t> edge_load;
    std::vector<std::bitset<MAX_NUM_PARTITION> > vertex_partition_matrix;
#endif
public:
    static const int MAX_SHRINK = 100;
    //std::ofstream part_file;
    std::vector<std::ofstream> part_files;
    explicit Greedy(Globals& GLOBALS);
    void perform_partitioning();
    void do_greedy(const std::vector<edge_t>& edges);
    int hash_vertex(uint32_t vertex);
    void write_edge(edge_t e, int p);  

#ifdef STATS
    std::vector<uint64_t>& get_edge_load();
    std::vector<std::bitset<MAX_NUM_PARTITION>> get_vertex_partition_matrix();

    degree_hash_table_type dht;
    std::vector<size_t> proc_num_edges;
#endif
};

#endif //HDRFPP__GREEDY_HPP_
