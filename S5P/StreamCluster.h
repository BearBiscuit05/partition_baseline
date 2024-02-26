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
    std::vector<int> clusterS; // map node to clu    
    std::vector<int> clusterB; // map node to clu        
    std::vector<int> degree;
    std::vector<int> degree_S;
    std::vector<int> volumeS; // cap of clu S     
    std::vector<int> volumeB; // cap of clu B    
    std::vector<int> clusterList_S;
    std::vector<int> clusterList_B;
    std::vector<bool> isInB;
    

    std::shared_ptr<std::vector<int>> cluPtrB;
    std::shared_ptr<std::vector<int>> cluPtrS;

    double maxVolume;
    CountMinSketch c;

    StreamCluster();
    void startStreamCluster();
    void computeHybridInfo();
    void calculateDegree();
    std::shared_ptr<std::vector<int>> getClusterPtrB();
    std::shared_ptr<std::vector<int>> getClusterPtrS();
    int getEdgeNum(int cluster1, int cluster2);
    void createCluList(std::vector<int>&,std::vector<int>&);
    
};




#endif // STREAM_CLUSTER_H
