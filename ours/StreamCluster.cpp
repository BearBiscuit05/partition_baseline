#include "StreamCluster.h"
#include <iostream>
#include <algorithm>
#include <fstream>

// StreamCluster::StreamCluster() :c(0.1, 0.01) {}

StreamCluster::StreamCluster() 
    : c(0.1, 0.01) {
    this->cluster_B.resize(size_t(config.vCount),-1);
    this->cluster_S.resize(size_t(config.vCount),-1);
    this->volume_B.resize(size_t(config.vCount),0);
    this->volume_S.resize(size_t(config.vCount),0);
    maxVolume = config.getMaxClusterVolume();
    degree.resize(config.vCount,0);
    degree_S.resize(config.vCount,0);
    calculateDegree();
    Triplet tmp = {-1,-1,0};
    this->cacheData.resize(BATCH,tmp);
}

void StreamCluster::startStreamCluster() {
    double averageDegree = config.getAverageDegree();
    int clusterID_B = 0;
    int clusterID_S = 0;
    int clusterNUM = config.vCount;
    std::string inputGraphPath = config.inputGraphPath;
    std::string line;
    std::pair<int,int> edge(-1,-1);
    this->isInB.resize(config.eCount,false);
    TGEngine tgEngine(inputGraphPath,config.vCount,config.eCount);
    int cachePtr = 0;
    std::vector<std::unordered_map<std::string , int>> maplist;

    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        if (degree[src] >= config.tao * averageDegree && degree[dest] >= config.tao * averageDegree) {
            this->isInB[tgEngine.readPtr/2] = true;
            auto& com_src = cluster_B[src];
            auto& com_dest = cluster_B[dest];
            if(com_src == -1) {
                com_src = clusterID_B;
                volume_B[com_src] += degree[src];
                ++clusterID_B;
            }
            if(com_dest == -1) {
                com_dest = clusterID_B;
                volume_B[com_dest] += degree[src];
                ++clusterID_B;
            }
            

            
            auto& vol_src = volume_B[com_src];
            auto& vol_dest = volume_B[com_dest];


            auto real_vol_src = vol_src - degree[src];
            auto real_vol_dest = vol_dest - degree[dest];

            if (real_vol_src <= real_vol_dest && vol_dest + degree[src] <= maxVolume) {
                vol_src -= degree[src];
               	vol_dest += degree[src];
               	cluster_B[src] = cluster_B[dest];
            } else if (real_vol_dest <= real_vol_src && vol_src + degree[dest] <= maxVolume) {
                vol_src -= degree[dest];
               	vol_dest += degree[dest];
               	cluster_B[dest] = cluster_B[src];
            }
        } else {
            if (cluster_S[src] == -1) 
                cluster_S[src] = clusterID_S++;
            if (cluster_S[dest] == -1) 
                cluster_S[dest] = clusterID_S++;
            degree_S[src]++;
            degree_S[dest]++;
            std::string t;
            volume_S[cluster_S[src] ]++;
            volume_S[cluster_S[dest]]++;     
            if (volume_S[cluster_S[src]] < maxVolume && volume_S[cluster_S[dest]] < maxVolume) {
                int minVid = (volume_S[cluster_S[src]] < volume_S[cluster_S[dest]] ? src : dest);
                int maxVid = (src == minVid ? dest : src);
                if ((volume_S[cluster_S[maxVid]] + degree_S[minVid]) <= maxVolume) {
                    volume_S[cluster_S[maxVid]] += degree_S[minVid];
                    volume_S[cluster_S[minVid]] -= degree_S[minVid];
                    cluster_S[minVid] = cluster_S[maxVid];
                }    
            } 
        }
    }

    std::vector<std::pair<uint64_t, uint32_t>> sorted_communities;
    for (size_t i = 0; i < volume_B.size(); ++i)
    {
        if (volume_B[i] != 0)
            sorted_communities.emplace_back(volume_B[i], i);
    }
    volume_B.clear();  
    std::sort(sorted_communities.rbegin(), sorted_communities.rend()); // sort in descending order

    for (int i = 0; i < sorted_communities.size(); ++i) {
        clusterList_B.push_back(sorted_communities[i].second);
    }
    sorted_communities.clear();
    for (size_t i = 0; i < volume_S.size(); ++i)
    {
        if (volume_S[i] != 0)
            sorted_communities.emplace_back(volume_S[i], i);
    }
    volume_S.clear(); 
    std::sort(sorted_communities.rbegin(), sorted_communities.rend()); // sort in descending order
    for (int i = 0; i < sorted_communities.size(); ++i) {
        clusterList_S.push_back(sorted_communities[i].second + config.vCount);
    }
    sorted_communities.clear();
}


void StreamCluster::startStreamCluster_MAP() {
    double averageDegree = config.getAverageDegree();
    int clusterID_B = 0;
    int clusterID_S = 0;
    int clusterNUM = config.vCount;
    std::cout << "start read Streaming Clustring..." << std::endl;
    std::string inputGraphPath = config.inputGraphPath;
    std::string line;
    std::pair<int,int> edge(-1,-1);
    this->isInB.resize(config.eCount,false);
    TGEngine tgEngine(inputGraphPath,config.vCount,config.eCount);
    int cachePtr = 0;
    std::vector<std::unordered_map<std::string , int>> maplist;
    for (int i = 0 ; i < THREADNUM ; i++) {
        std::unordered_map<std::string , int> mapTmp;
        maplist.emplace_back(mapTmp);
    }
    while (-1 != tgEngine.readline(edge)) {
        if (cachePtr + 3 >= BATCH) {
            mergeMap(maplist,cachePtr);
        }
        int src = edge.first;
        int dest = edge.second;
        if (degree[src] >= config.tao * averageDegree && degree[dest] >= config.tao * averageDegree) {
            this->isInB[tgEngine.readPtr/2] = true;
            auto& com_src = cluster_B[src];
            auto& com_dest = cluster_B[dest];
            if(com_src == -1) {
                com_src = clusterID_B;
                volume_B[com_src] += degree[src];
                ++clusterID_B;
            }
            if(com_dest == -1) {
                com_dest = clusterID_B;
                volume_B[com_dest] += degree[src];
                ++clusterID_B;
            }
            auto& vol_src = volume_B[com_src];
            auto& vol_dest = volume_B[com_dest];

            auto real_vol_src = vol_src - degree[src];
            auto real_vol_dest = vol_dest - degree[dest];

            if (real_vol_src <= real_vol_dest && vol_dest + degree[src] <= maxVolume) {
                vol_src -= degree[src];
               	vol_dest += degree[src];
               	cluster_B[src] = cluster_S[dest];
            } else if (real_vol_dest <= real_vol_src && vol_src + degree[dest] <= maxVolume) {
                vol_src -= degree[dest];
               	vol_dest += degree[dest];
               	cluster_B[dest] = cluster_B[src];
            }
            this->cacheData[cachePtr].src = src;
            this->cacheData[cachePtr++].dst = dest;
        } else {
            if (cluster_S[src] == -1) 
                cluster_S[src] = clusterID_S++;
            if (cluster_S[dest] == -1) 
                cluster_S[dest] = clusterID_S++;
            degree_S[src]++;
            degree_S[dest]++;

            if (cluster_S[src] >= volume_S.size() || cluster_S[dest] >= volume_S.size()) 
                volume_S.resize(volume_S.size() + 0.1 * config.vCount, 0);

            volume_S[cluster_S[src] ]++;
            volume_S[cluster_S[dest]]++;
            if (volume_S[cluster_S[src]] < maxVolume && volume_S[cluster_S[dest]] < maxVolume) {
                int minVid = (volume_S[cluster_S[src]] < volume_S[cluster_S[dest]] ? src : dest);
                int maxVid = (src == minVid ? dest : src);
                if ((volume_S[cluster_S[maxVid]] + degree_S[minVid]) <= maxVolume) {
                    volume_S[cluster_S[maxVid]] += degree_S[minVid];
                    volume_S[cluster_S[minVid]] -= degree_S[minVid];
                    cluster_S[minVid] = cluster_S[maxVid];
                }    
                this->cacheData[cachePtr].src = src;
                this->cacheData[cachePtr].dst = dest;
                this->cacheData[cachePtr++].flag = 1;
                if (cluster_B[src] !=-1) {
                    this->cacheData[cachePtr].src = src;
                    this->cacheData[cachePtr].dst = dest;
                    this->cacheData[cachePtr++].flag = 2;
                }
                if (cluster_B[dest] != -1) {
                    this->cacheData[cachePtr].src = src;
                    this->cacheData[cachePtr].dst = dest;
                    this->cacheData[cachePtr++].flag = 3;
                } 
            } else {
                continue;
            }
        }
    }
    mergeMap(maplist,cachePtr);
    for (int i = 1 ;  i < THREADNUM ; i++) {
        for(auto& m : maplist[i]) {
            maplist[0][m.first] += m.second;
        }
    }

    this->innerAndCutEdge = std::move(maplist[0]);
    maplist = std::vector<std::unordered_map<std::string , int>>();

    for (int i = 0; i < volume_B.size(); ++i) {
        if (volume_B[i] != 0)
            clusterList_B.push_back(i);
    }
    volume_B.clear();  

    for (int i = 0; i < volume_S.size(); ++i) {
        if (volume_S[i] != 0)
            clusterList_S.push_back(i + config.vCount);
    }
    volume_S.clear();  
    config.clusterBSize = config.vCount;
}

void StreamCluster::mergeMap(std::vector<std::unordered_map<std::string , int>>& maplist,int& cachePtr) {
#pragma omp parallel for
    for (int i = 0 ;  i < cachePtr ; i++) {
        int flag = this->cacheData[i].flag;
        int tid = omp_get_thread_num();
        if(flag == 0) {
            maplist[tid][std::to_string(this->cluster_B[this->cacheData[i].src]) + "," + std::to_string(this->cluster_B[this->cacheData[i].dst])] += 1;
        } else if (flag == 1) {
            maplist[tid][std::to_string(this->cluster_S[this->cacheData[i].src] + config.vCount) + "," + std::to_string(this->cluster_S[this->cacheData[i].dst] + config.vCount)] += 1;
        } else if (flag == 2) {
            maplist[tid][std::to_string(this->cluster_S[this->cacheData[i].src] + config.vCount) + "," + std::to_string(this->cluster_B[this->cacheData[i].dst])] += 1;
        } else {
            maplist[tid][std::to_string(this->cluster_B[this->cacheData[i].src]) + "," + std::to_string(this->cluster_S[cacheData[i].dst] +  config.vCount)] += 1;
        }
    }
    cachePtr = 0;
}

void StreamCluster::computeHybridInfo() {
    std::string inputGraphPath = config.inputGraphPath;
    std::pair<int,int> edge(-1,-1);
    TGEngine tgEngine(inputGraphPath,config.vCount,config.eCount); 
    std::string t;
    uint64_t key;
    uint64_t c1;
    uint64_t c2;
    while (-1 != tgEngine.readline(edge)) {
        int src = edge.first;
        int dest = edge.second;
        
        if (!(this->isInB[tgEngine.readPtr/2])) {
            key = (cluster_S[src] + config.vCount) * config.vCount + (cluster_S[dest] + config.vCount);
            c.update(key, 1);

            key = (cluster_S[dest] + config.vCount) * config.vCount + (cluster_S[src] + config.vCount);
            c.update(key, 1);
            if (cluster_B[src] !=-1) {
                c2 = cluster_B[dest];
                c1 = cluster_S[src] + config.vCount;
                key = c1 * config.vCount + c2;
                c.update(key, 1);


                key = c2 * config.vCount + c1;
                c.update(key, 1);
            }
            if (cluster_B[dest] != -1) {
                c1 = cluster_B[src];
                c2 = cluster_S[dest] + config.vCount;
                key = c1 * config.vCount + c2 ;
                c.update(key, 1);

                key = c2 * config.vCount + c1 ;
                c.update(key, 1);
            }   
        } else {
            c1 = cluster_B[src];
            c2 = cluster_B[dest];
            key = c1*config.vCount + c2;
            this->c.update(key, 1);

            key = c2*config.vCount + c1;
            this->c.update(key, 1);
        }
    }
    
}

void StreamCluster::calculateDegree() {
    std::pair<int,int> edge(-1,-1);
    std::string inputGraphPath = config.inputGraphPath;
    TGEngine tgEngine(inputGraphPath,config.vCount,config.eCount);  
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

std::vector<int> StreamCluster::getClusterList_B() {
    return clusterList_B;
}

std::vector<int> StreamCluster::getClusterList_S() {
    return clusterList_S;
}



void StreamCluster::outputClusterSizeInfo() {
    std::string filename = "headcluster.csv";
    std::unordered_map<uint32_t, uint32_t> mp;
    for (int i = 0; i < this->volume_B.size(); ++i) {
        if (this->volume_B[i] != 0)
            mp[this->volume_B[i]]++;
    }
    std::ofstream outputFile(filename);
    int j = 0;
    for (auto &t : mp) {
        outputFile << t.first << "," << t.second;
        if (j < mp.size() - 1) {
            outputFile << "\n";
        }
        j++;
    }
    outputFile.close();

    filename = "tailcluster.csv";
    mp.clear();
    for (int i = 0; i < this->volume_S.size(); ++i) {
        if (this->volume_S[i] != 0)
            mp[this->volume_S[i]]++;
    }
    std::ofstream outputFile2(filename);
    j = 0;
    for (auto &t : mp) {
        outputFile2 << t.first << "," << t.second;
        if (j < mp.size() - 1) {
            outputFile2 << "\n";
        }
        j++;
    }
    outputFile2.close();
}




