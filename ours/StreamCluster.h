#ifndef STREAM_CLUSTER_H
#define STREAM_CLUSTER_H
#include "globalConfig.h"
# include "cm_sketch.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include "readGraph.h"

struct Triplet {
    int src;
    int dst;
    char flag;
};

class StreamCluster {
public:
    std::vector<int> cluster_S;     
    std::vector<int> cluster_B;     
    std::vector<int> degree;
    std::vector<int> degree_S;
    std::vector<uint32_t> volume_S;     
    std::vector<uint32_t> volume_B;     
    std::unordered_map<std::string , int> innerAndCutEdge;
    std::vector<int> clusterList_S;
    std::vector<int> clusterList_B;
    CountMinSketch c;

    double maxVolume;
    int maxVolume_B;
    int maxVolume_S;
    std::string graphType;
    GlobalConfig config;
    StreamCluster();
    StreamCluster(GlobalConfig& config);
    void startStreamCluster();
    void startStreamCluster_MAP();
    void computeHybridInfo();
    void calculateDegree();
    int getEdgeNum(int cluster1, int cluster2);
    std::vector<bool> isInB;
    std::vector<int> getClusterList_B();
    std::vector<int> getClusterList_S();
    std::vector<Triplet> cacheData;
    void outputClusterSizeInfo();
    void mergeMap(std::vector<std::unordered_map<std::string , int>>& maplist,int& cachePtr);
};




#endif // STREAM_CLUSTER_H
