#ifndef STREAM_CLUSTER_H
#define STREAM_CLUSTER_H
#include "globalConfig.h"
# include "cm_sketch.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include "readGraph.h"


class StreamCluster {
public:
    std::vector<int> clusterS;     
    std::vector<int> clusterB;     
    std::vector<int> degree;
    std::vector<int> degree_S;
    std::vector<uint32_t> volumeS; // cap of clu S     
    std::vector<uint32_t> volumeB; // cap of clu B    
    std::unordered_map<std::string , int> innerAndCutEdge;
    std::vector<int> clusterList_S;
    std::vector<int> clusterList_B;
    CountMinSketch c;

    double maxVolume;
    int maxVolume_B;
    int maxVolume_S;
    std::string graphType;
    StreamCluster();
    void startStreamCluster();
    void computeHybridInfo();
    void calculateDegree();
    int getEdgeNum(int cluster1, int cluster2);
    std::vector<bool> isInB;
    std::vector<int> getClusterList_B();
    std::vector<int> getClusterList_S();
};




#endif // STREAM_CLUSTER_H
