#pragma once

#include <utility>
#include <chrono>
#include <stdint.h>
#include <sys/stat.h>

#include <gflags/gflags.h>
#include <glog/logging.h>

#define rep(i, n) for (int i = 0; i < (int)(n); ++i)
#define repv(i, n) for (vid_t i = 0; i < n; ++i)

DECLARE_int32(p);
DECLARE_uint64(memsize);
DECLARE_string(filename);
DECLARE_string(filetype);
DECLARE_bool(inmem);
DECLARE_double(sample_ratio);

typedef uint32_t vid_t;
struct edge_t {
    vid_t first, second;
    edge_t() : first(0), second(0) {}
    edge_t(vid_t first, vid_t second) : first(first), second(second) {}
};

void writea(int f, char *buf, size_t nbytes);

inline std::string binedgelist_name(const std::string &basefilename)
{
    std::stringstream ss;
    ss << basefilename << ".binedgelist";
    return ss.str();
}
inline std::string shuffled_binedgelist_name(const std::string &basefilename)
{
    std::stringstream ss;
    ss << basefilename << ".shuffled.binedgelist";
    return ss.str();
}
inline std::string degree_name(const std::string &basefilename)
{
    std::stringstream ss;
    ss << basefilename << ".degree";
    return ss.str();
}
inline std::string partitioned_name(const std::string &basefilename)
{
    std::stringstream ss;
    ss << basefilename << ".edgepart." << FLAGS_p; // chenzi: add partition number to the output file
    return ss.str();
}
inline bool is_exists(const std::string &name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

class Timer
{
private:
    std::chrono::system_clock::time_point t1, t2;
    double total;

public:
    Timer() : total(0) {}
    void reset() { total = 0; }
    void start() { t1 = std::chrono::system_clock::now(); }
    void stop()
    {
        t2 = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = t2 - t1;
        total += diff.count();
    }
    double get_time() { return total; }
};
