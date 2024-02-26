#ifndef CLUSTER_GAME_TASK_H
#define CLUSTER_GAME_TASK_H

#include "StreamCluster.h"


class ClusterGameTask {
public:
    StreamCluster* streamCluster;
    std::string graphType;
    int roundCnt;
    double beta;
    double beta_B;
    double beta_S;

    std::vector<int> cluster;
    std::vector<int> cluster_B;
    std::vector<int> cluster_S;
    std::vector<double> partitionLoad;
    std::vector<int> newClusterNeighbours;
    std::unordered_map<int, int> cutCostValue;
    std::unordered_map<int, std::unordered_set<int>> clusterNeighbours;
    std::vector<int>* clusterPartition;
    
    ClusterGameTask() {};
    ClusterGameTask(StreamCluster& sc,std::vector<int>& clusterPartition);
    
    void call();
    void startGameDouble();
    void initGame();
    double computeCost(int clusterId, int partition, const std::string type);
    void resize_hyper(std::string graphType, int taskIds);
    void resize(std::string graphType, int taskIds);
    void startGameSingle();
};

#endif // CLUSTER_GAME_TASK_H
