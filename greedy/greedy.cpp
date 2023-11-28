#include "greedy.hpp"
#include <random>
#include <iostream>




int edge_to_proc_greedy (
    const int source, 
    const int target,
    bin_counts_type& src_degree,
    bin_counts_type& dst_degree,
    std::vector<size_t>& proc_num_edges,
    bool usehash = false,
    bool userecent = false) {
        size_t numprocs = proc_num_edges.size();

        // Compute the score of each proc.
        int best_proc = -1; 
        double maxscore = 0.0;
        double epsilon = 1.0; 
        std::vector<double> proc_score(numprocs); 
        size_t minedges = *std::min_element(proc_num_edges.begin(), proc_num_edges.end());
        size_t maxedges = *std::max_element(proc_num_edges.begin(), proc_num_edges.end());

        for (size_t i = 0; i < numprocs; ++i) {
          size_t sd = src_degree.get(i) + (usehash && (source % numprocs == i));
          size_t td = dst_degree.get(i) + (usehash && (target % numprocs == i));
          double bal = (maxedges - proc_num_edges[i])/(epsilon + maxedges - minedges);
          proc_score[i] = bal + ((sd > 0) + (td > 0));
        }
        maxscore = *std::max_element(proc_score.begin(), proc_score.end());

        std::vector<int> top_procs; 
        for (size_t i = 0; i < numprocs; ++i)
          if (std::fabs(proc_score[i] - maxscore) < 1e-5)
            top_procs.push_back(i);

        // Hash the edge to one of the best procs.
        typedef std::pair<int, int> edge_pair_type;
        const edge_pair_type edge_pair(std::min(source, target), 
            std::max(source, target));
        best_proc = top_procs[graph_hash::hash_edge(edge_pair) % top_procs.size()];

        //ASSERT_LT(best_proc, numprocs);
        if (userecent) {
          src_degree.clear();
          dst_degree.clear();
        }
        src_degree.set_bit(best_proc);
        dst_degree.set_bit(best_proc);
        ++proc_num_edges[best_proc];
        return best_proc;
};


void greedy_forwarder(void* object, std::vector<edge_t> edges);

DECLARE_bool(write_parts);
DECLARE_string(parts_filename);


Greedy::Greedy(Globals &GLOBALS) : globals(GLOBALS),dht(-1)
{
    seed = static_cast<double>(random()) / RAND_MAX;
    shrink = static_cast<int>(random()) % MAX_SHRINK;
    proc_num_edges.resize(globals.NUM_PARTITIONS,0);
    part_files.resize(globals.NUM_PARTITIONS);
    if (FLAGS_write_parts) {
        for (int i = 0 ; i < globals.NUM_PARTITIONS ; i++) {
            std::string fileName = FLAGS_parts_filename + "_" + std::to_string(i) + ".bin";
            part_files[i].open(fileName,std::ios::out | std::ios::binary);
        }
        //part_file.open(FLAGS_parts_filename + ".bin",std::ios::out | std::ios::binary);
    }
#ifdef STATS
    vertex_partition_matrix.resize(globals.NUM_VERTICES);
    edge_load.resize(globals.NUM_PARTITIONS, 0);
#endif
}

void Greedy::perform_partitioning()
{
    globals.read_and_do(greedy_forwarder, this, "partitions with Rreedy");
    if (FLAGS_write_parts) {
        for (int i = 0 ; i < globals.NUM_PARTITIONS ; i++) {
            part_files[i].close();
        }
        
    }
}

void Greedy::write_edge(edge_t e, int p){
    part_files[p].write(reinterpret_cast<char*>(&e.first), sizeof(e.first));
    part_files[p].write(reinterpret_cast<char*>(&e.second), sizeof(e.second));
}


void Greedy::do_greedy(const std::vector<edge_t>& edges)
{
    for (const edge_t& e : edges)
    {
        auto source = e.first;
        auto target = e.second;

        dht[source]; dht[target];
        int p_id = edge_to_proc_greedy(source, target, dht[source], dht[target], proc_num_edges);


    if (FLAGS_write_parts){
            write_edge(e, p_id);
        }

#ifdef STATS
        vertex_partition_matrix[e.first][p_id] = true;
        vertex_partition_matrix[e.second][p_id] = true;

        auto& load = ++edge_load[p_id];
        if (load < min_load) min_load = load;
        if (load > max_load) max_load = load;
#endif
    }
}

int Greedy::hash_vertex(uint32_t v) 
{
   return abs(static_cast<int>(static_cast<uint32_t>(v * seed * shrink) % globals.NUM_PARTITIONS)); 
}

#ifdef STATS
std::vector<uint64_t>& Greedy::get_edge_load()
{
    return edge_load;
}

std::vector<std::bitset<MAX_NUM_PARTITION>> Greedy::get_vertex_partition_matrix()
{
    return vertex_partition_matrix;
}
#endif

void greedy_forwarder(void* object, std::vector<edge_t> edges)
{
    static_cast<Greedy*>(object)->do_greedy(edges);
}
