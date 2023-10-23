#include "random.hpp"
#include <random>
#include <iostream>
void random_forwarder(void* object, std::vector<edge_t> edges);

DECLARE_bool(write_parts);
DECLARE_string(parts_filename);


Random::Random(Globals &GLOBALS) : globals(GLOBALS)
{
    seed = static_cast<double>(random()) / RAND_MAX;
    shrink = static_cast<int>(random()) % MAX_SHRINK;

    if (FLAGS_write_parts) {
        part_file.open(FLAGS_parts_filename + ".bin",std::ios::out | std::ios::binary);
    }
#ifdef STATS
    vertex_partition_matrix.resize(globals.NUM_VERTICES);
    edge_load.resize(globals.NUM_PARTITIONS, 0);
#endif
}

void Random::perform_partitioning()
{
    globals.read_and_do(random_forwarder, this, "partitions with Random");
    if (FLAGS_write_parts) {
        part_file.close();
    }
}

void Random::write_edge(edge_t e, int p){
    part_file.write(reinterpret_cast<char*>(&e.first), sizeof(e.first));
    part_file.write(reinterpret_cast<char*>(&e.second), sizeof(e.second));
    part_file.write(reinterpret_cast<char*>(&p), sizeof(p));
    // part_file << e.first << "\t" << e.second << "\t" << p << std::endl;
}


void Random::do_random(const std::vector<edge_t>& edges)
{
    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<int> dist(0, this->globals.NUM_PARTITIONS - 1);
    for (const edge_t& e : edges)
    {
        auto u = e.first;
        auto v = e.second;

        int p_id = dist(gen);

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

int Random::hash_vertex(uint32_t v) 
{
   return abs(static_cast<int>(static_cast<uint32_t>(v * seed * shrink) % globals.NUM_PARTITIONS)); 
}

#ifdef STATS
std::vector<uint64_t>& Random::get_edge_load()
{
    return edge_load;
}

std::vector<std::bitset<MAX_NUM_PARTITION>> Random::get_vertex_partition_matrix()
{
    return vertex_partition_matrix;
}
#endif

void random_forwarder(void* object, std::vector<edge_t> edges)
{
    static_cast<Random*>(object)->do_random(edges);
}
