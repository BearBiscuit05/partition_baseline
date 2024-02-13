#ifndef PARTITIONER_H
#define PARTITIONER_H


#include "ClusterGameTask.h"

class Partitioner {
public:
    StreamCluster* streamCluster;
    std::vector<int> gameRoundCnt_hybrid;
    std::vector<int>  gameRoundCnt_inner;
    std::vector<int> partitionLoad;
    std::vector<std::vector<char>> v2p; 
    std::vector<int> clusterPartition;
    GlobalConfig config;
    Partitioner();
    Partitioner(StreamCluster& streamCluster,GlobalConfig config);
    void performStep();
    double getReplicateFactor();
    double getLoadBalance();
    void startStackelbergGame();
};

#endif // PARTITIONER_H
