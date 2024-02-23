#include "StreamCluster.h"
#include <iostream>
#include <algorithm>
#include <fstream>

// StreamCluster::StreamCluster() :c(0.1, 0.01) {}

StreamCluster::StreamCluster() 
    : c(0.1, 0.01) {
    this->clusterB.resize(config.vCount,-1);
    this->clusterS.resize(config.vCount,-1);
    this->volumeB.resize(config.vCount,0);
    this->volumeS.resize(config.vCount,0);
    this->degree.resize(config.vCount,0);
    this->degree_S.resize(config.vCount,0);
    this->maxVolume = config.getMaxClusterVolume();
    calculateDegree();
}

void StreamCluster::startStreamCluster() {
    double averageDegree = config.getAverageDegree();
    int clusterID_B = 0;
    int clusterID_S = 0;
    std::pair<int,int> edge(-1,-1);
    this->isInB.resize(config.eCount,false);
    TGEngine tgEngine(config.inputGraphPath,config.vCount,config.eCount);

    // read stream edges
    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        if (degree[src] >= config.tao * averageDegree && degree[dest] >= config.tao * averageDegree) {
            this->isInB[tgEngine.readPtr/2] = true;
            int& com_src = clusterB[src];
            int& com_dest = clusterB[dest];
            
            // init clusterID for node
            if(com_src == -1) {
                com_src = clusterID_B;
                volumeB[com_src] += degree[src];
                ++clusterID_B;
            }
            if(com_dest == -1) {
                com_dest = clusterID_B;
                volumeB[com_dest] += degree[dest];
                ++clusterID_B;
            }
            

            uint32_t& vol_src = volumeB[com_src];
            uint32_t& vol_dest = volumeB[com_dest];


            auto real_vol_src = vol_src - degree[src];
            auto real_vol_dest = vol_dest - degree[dest];

            if (real_vol_src <= real_vol_dest && vol_dest + degree[src] <= maxVolume) {
                vol_src -= degree[src];
               	vol_dest += degree[src];
               	clusterB[src] = clusterB[dest];
            } else if (real_vol_dest <= real_vol_src && vol_src + degree[dest] <= maxVolume) {
                vol_src -= degree[dest];
               	vol_dest += degree[dest];
               	clusterB[dest] = clusterB[src];
            }
        } else {
            if (clusterS[src] == -1) 
                clusterS[src] = clusterID_S++;
            if (clusterS[dest] == -1) 
                clusterS[dest] = clusterID_S++;
            degree_S[src]++;
            degree_S[dest]++;

            volumeS[clusterS[src] ]++;
            volumeS[clusterS[dest]]++;     
            if (volumeS[clusterS[src]] < maxVolume && volumeS[clusterS[dest]] < maxVolume) {
                int minVid = (volumeS[clusterS[src]] < volumeS[clusterS[dest]] ? src : dest);
                int maxVid = (src == minVid ? dest : src);
                if ((volumeS[clusterS[maxVid]] + degree_S[minVid]) <= maxVolume) {
                    volumeS[clusterS[maxVid]] += degree_S[minVid];
                    volumeS[clusterS[minVid]] -= degree_S[minVid];
                    clusterS[minVid] = clusterS[maxVid];
                }    
            } 
        }
    }

    std::vector<std::pair<uint64_t, uint32_t>> sorted_communities;
    for (size_t i = 0; i < volumeB.size(); ++i)
    {
        if (volumeB[i] != 0)
            sorted_communities.emplace_back(volumeB[i], i);
    }
    volumeB.clear();  
    std::sort(sorted_communities.rbegin(), sorted_communities.rend()); // sort in descending order

    for (int i = 0; i < sorted_communities.size(); ++i) {
        clusterList_B.push_back(sorted_communities[i].second);
    }
    sorted_communities.clear();
    for (size_t i = 0; i < volumeS.size(); ++i)
    {
        if (volumeS[i] != 0)
            sorted_communities.emplace_back(volumeS[i], i);
    }
    volumeS.clear(); 
    std::sort(sorted_communities.rbegin(), sorted_communities.rend()); // sort in descending order
    for (int i = 0; i < sorted_communities.size(); ++i) {
        clusterList_S.push_back(sorted_communities[i].second + config.vCount);
    }
    sorted_communities.clear();
}

void StreamCluster::computeHybridInfo() {
    TGEngine tgEngine(config.inputGraphPath,config.vCount,config.eCount); 
    uint64_t key,c1,c2;
    std::pair<int,int> edge;

    // read for stream edge
    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        
        if (!(this->isInB[tgEngine.readPtr/2])) {
            key = (clusterS[src] + config.vCount) * config.vCount + (clusterS[dest] + config.vCount);
            c.update(key, 1);

            key = (clusterS[dest] + config.vCount) * config.vCount + (clusterS[src] + config.vCount);
            c.update(key, 1);
            if (clusterB[src] !=-1) {
                c2 = clusterB[dest];
                c1 = clusterS[src] + config.vCount;
                key = c1 * config.vCount + c2;
                c.update(key, 1);
                key = c2 * config.vCount + c1;
                c.update(key, 1);
            }
            if (clusterB[dest] != -1) {
                c1 = clusterB[src];
                c2 = clusterS[dest] + config.vCount;
                key = c1 * config.vCount + c2 ;
                c.update(key, 1);
                key = c2 * config.vCount + c1 ;
                c.update(key, 1);
            }   
        } else {
            c1 = clusterB[src];
            c2 = clusterB[dest];
            key = c1*config.vCount + c2;
            this->c.update(key, 1);

            key = c2*config.vCount + c1;
            this->c.update(key, 1);
        }
    }
    
}

void StreamCluster::calculateDegree() {
    // compute node degree
    std::pair<int,int> edge(-1,-1);
    TGEngine tgEngine(config.inputGraphPath,config.vCount,config.eCount);  
    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        degree[src] ++;
        degree[dest] ++;
    }
}

int StreamCluster::getEdgeNum(int cluster1, int cluster2) {
    uint64_t index = cluster1 * config.vCount + cluster2;
    return this->c.estimate(index); 
}


// TODO:
std::vector<int> StreamCluster::getClusterList_B() {
    return clusterList_B;
}

std::vector<int> StreamCluster::getClusterList_S() {
    return clusterList_S;
}




