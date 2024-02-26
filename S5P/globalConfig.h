#ifndef GLOBALCONFIG_H
#define GLOBALCONFIG_H

#include "common.h"
class GlobalConfig {
public:
    long long eCount;
    double beta;
    double tao;
    double alpha;
    int k;
    int batchSize;
    int threads;
    int partitionNum;
    int vCount;
    int clusterBSize;


    std::string inputGraphPath;
    GlobalConfig() {};
    GlobalConfig(std::string filepath);
    double getMaxClusterVolume();
    double getAverageDegree();
};


extern GlobalConfig config;
#endif // GLOBALCONFIG_H
