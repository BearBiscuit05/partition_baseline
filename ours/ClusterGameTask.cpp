#include "ClusterGameTask.h"
#include "StreamCluster.h"
#include "globalConfig.h"
#include <algorithm>


ClusterGameTask::ClusterGameTask(StreamCluster& sc,std::vector<int>& clusterPartition)
    : streamCluster(&sc){
    std::vector<int> clusterList = (graphType == "B" ? streamCluster->getClusterList_B() : streamCluster->getClusterList_S());
    int batchSize = streamCluster->config.batchSize;
    this->config = &streamCluster->config;
    this->partitionLoad.resize(this->streamCluster->config.partitionNum,0);
    this->clusterPartition=&clusterPartition;
    this->cutCostValue = std::unordered_map<int, int>();
    this->roundCnt = 0;
    this->newClusterNeighbours = std::vector<int>(2*config->vCount, 0);
}

void ClusterGameTask::startGameSingle() {
    int partition = 0;
    for (int clusterId : this->cluster) {
        double minLoad = this->config->eCount;
        for (int i = 0; i < this->config->partitionNum; i++) {
            if (partitionLoad[i] < minLoad) {
                minLoad = partitionLoad[i];
                partition = i;
            }
        }
        (*clusterPartition)[clusterId] = partition;
        partitionLoad[partition] += this->streamCluster->getEdgeNum(clusterId, clusterId);
    }

    double sizePart = 0.0, cutPart = 0.0;
    for (int cluster1 : this->cluster) {
        sizePart += streamCluster->getEdgeNum(cluster1, cluster1);
        for (int cluster2 : this->cluster) {
            int innerCut = 0;
            
            if (cluster1 != cluster2) {
                innerCut = streamCluster->getEdgeNum(cluster1, cluster2);
                this->newClusterNeighbours[cluster1] += innerCut;
                cutPart += innerCut;
            }
            auto it = this->cutCostValue.find(cluster1);
            if (it == this->cutCostValue.end())
                this->cutCostValue[cluster1] = 0;
            cutCostValue[cluster1] += innerCut;
        }

        for (int cluster2 : this->cluster_S) {
            int innerCut = 0;
            if (cluster1 != cluster2) {
                innerCut = streamCluster->getEdgeNum(cluster1, cluster2);
                this->newClusterNeighbours[cluster1] += innerCut;
            }
            if(cutCostValue.find(cluster1) == cutCostValue.end()) {
                cutCostValue[cluster1]  = 0;
            }
            cutCostValue[cluster1] += innerCut;
        }
    }

    this->beta = (double)this->streamCluster->config.partitionNum  / (sizePart * sizePart + 1.0)  * ((double)cutPart + sizePart);
    bool finish = false;
    while (true) {
        finish = true;
        for (int clusterId : this->cluster) {
            double minCost = std::numeric_limits<double>::max();
            int minPartition = (*clusterPartition)[clusterId];
            for (int j = 0; j < config->partitionNum; j++) {
                double cost = computeCost(clusterId, j, "BS");
                if (cost <= minCost) {
                    minCost = cost;
                    minPartition = j;
                }
            }

            if (minPartition != (*clusterPartition)[clusterId]) {
                finish = false;
                partitionLoad[minPartition] += streamCluster->getEdgeNum(clusterId, clusterId);
                partitionLoad[(*clusterPartition)[clusterId]] -= streamCluster->getEdgeNum(clusterId, clusterId);
                (*clusterPartition)[clusterId] = minPartition;
            }
        }
        roundCnt++;
        if (finish) {break;}
    }
}


ClusterGameTask::ClusterGameTask(std::string graphType, int taskId, StreamCluster& sc)
    : graphType(graphType), streamCluster(&sc){
    std::vector<int> clusterList = (graphType == "B" ? streamCluster->getClusterList_B() : streamCluster->getClusterList_S());
    int batchSize = streamCluster->config.batchSize;
    int begin = batchSize * taskId;
    int end = std::min(batchSize * (taskId + 1), static_cast<int>(clusterList.size()));
    this->cluster.assign(clusterList.begin() + begin, clusterList.begin() + end);
    this->config = &streamCluster->config;
    this->cutCostValue = std::unordered_map<int, int>();
    this->partitionLoad.resize(this->streamCluster->config.partitionNum,0);
}

ClusterGameTask::ClusterGameTask(std::string graphType, StreamCluster& sc, int taskIds)
        : graphType(graphType), streamCluster(&sc){
    std::vector<int> clusterList_B = streamCluster->getClusterList_B();
    std::vector<int> clusterList_S = streamCluster->getClusterList_S();

    int batchSize = streamCluster->config.batchSize;
    int begin = batchSize * taskIds;
    int end = std::min(batchSize * (taskIds + 1), static_cast<int>(clusterList_B.size()));
    this->cluster_B.assign(clusterList_B.begin() + begin, clusterList_B.begin() + end);
    this->config = &streamCluster->config;
    
    begin = batchSize * taskIds;
    end = std::min(batchSize * (taskIds + 1), static_cast<int>(clusterList_S.size()));
    this->cluster_S.assign(clusterList_S.begin() + begin, clusterList_S.begin() + end);

    this->partitionLoad.resize(this->streamCluster->config.partitionNum,0);
    this->cutCostValue = std::unordered_map<int, int>();
    this->clusterNeighbours = std::unordered_map<int, std::unordered_set<int>>();
}

void ClusterGameTask::resize_hyper(std::string graphType,int taskIds) {
    std::vector<int> clusterList_B = streamCluster->getClusterList_B();
    std::vector<int> clusterList_S = streamCluster->getClusterList_S();
    int batchSize = streamCluster->config.batchSize;
    int begin = batchSize * taskIds;
    int end = std::min(batchSize * (taskIds + 1), static_cast<int>(clusterList_B.size()));
    this->cluster_B.assign(clusterList_B.begin() + begin, clusterList_B.begin() + end);
    this->graphType = graphType;
    begin = batchSize * taskIds;
    end = std::min(batchSize * (taskIds + 1), static_cast<int>(clusterList_S.size()));
    this->cluster_S.assign(clusterList_S.begin() + begin, clusterList_S.begin() + end);
    std::fill(this->partitionLoad.begin(), this->partitionLoad.end(), 0);
    this->cutCostValue = std::unordered_map<int, int>();
    this->clusterNeighbours = std::unordered_map<int, std::unordered_set<int>>();
    this->roundCnt = 0;
}

void ClusterGameTask::resize(std::string graphType, int taskId) {
    std::vector<int> clusterList = (graphType == "B" ? streamCluster->getClusterList_B() : streamCluster->getClusterList_S());
    this->graphType = graphType;
    int batchSize = streamCluster->config.batchSize;
    int begin = batchSize * taskId;
    int end = std::min(batchSize * (taskId + 1), static_cast<int>(clusterList.size()));
    this->cluster.assign(clusterList.begin() + begin, clusterList.begin() + end);
    
    this->cutCostValue = std::unordered_map<int, int>();
    std::fill(this->partitionLoad.begin(), this->partitionLoad.end(), 0);
    this->clusterNeighbours = std::unordered_map<int, std::unordered_set<int>>();
    this->roundCnt = 0;
}

void ClusterGameTask::call() {
    try {
        if (graphType == "hybrid") {
            this->startGameDouble();
        } else if (graphType == "B") {
            this->startGameSingle();
        } else if (graphType == "S") {
            this->startGameSingle();
        } else {
            std::cout << "graphType error" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}


void ClusterGameTask::initGame() {
    int partition = 0;
    for (int clusterId : this->cluster) {
        double minLoad = this->config->eCount;
        for (int i = 0; i < this->config->partitionNum; i++) {
            if (partitionLoad[i] < minLoad) {
                minLoad = partitionLoad[i];
                partition = i;
            }
        }
        (*clusterPartition)[clusterId] = partition;
        partitionLoad[partition] += this->streamCluster->getEdgeNum(clusterId, clusterId);
    }
}


void ClusterGameTask::startGameDouble() {

    int partition = 0;
    for (int clusterId : this->cluster_B) {
        double minLoad = config->eCount;
        for (int i = 0; i < config->partitionNum; i++) {
            if (partitionLoad[i] < minLoad) {
                minLoad = partitionLoad[i];
                partition = i;
            }
        }
        (*clusterPartition)[clusterId] = partition;
        partitionLoad[partition] += streamCluster->getEdgeNum(clusterId, clusterId);
    }

    for (int clusterId : this->cluster_S) {
        double minLoad = config->eCount;
        for (int i = 0; i < config->partitionNum; i++) {
            if (partitionLoad[i] < minLoad) {
                minLoad = partitionLoad[i];
                partition = i;
            }
        }
        (*clusterPartition)[clusterId] = partition;
        partitionLoad[partition] += streamCluster->getEdgeNum(clusterId, clusterId);
    }

    double sizePart_B = 0.0, cutPart_B = 0.0;
    double sizePart_S = 0.0, cutPart_S = 0.0;

    for (int cluster1 : this->cluster_B) {
        sizePart_B += streamCluster->getEdgeNum(cluster1, cluster1);
        for (int cluster2 : this->cluster_B) {
            int innerCut = 0;
            if (cluster1 != cluster2) {
                innerCut = streamCluster->getEdgeNum(cluster1, cluster2);
                this->newClusterNeighbours[cluster1] += innerCut;
                cutPart_B += innerCut;
            }
            auto it = cutCostValue.find(cluster1);
            if (it == cutCostValue.end())
                cutCostValue[cluster1] = 0;
            cutCostValue[cluster1] += innerCut;
        }

        for (int cluster2 : this->cluster_S) {
            int innerCut = 0;
            if (cluster1 != cluster2) {
                innerCut = streamCluster->getEdgeNum(cluster1, cluster2);
                this->newClusterNeighbours[cluster1] += innerCut;
            }
            if(cutCostValue.find(cluster1) == cutCostValue.end()) {
                cutCostValue[cluster1]  = 0;
            }
            cutCostValue[cluster1] += innerCut;
        }
    }
    
    this->beta_B = (double)this->streamCluster->config.partitionNum  / (sizePart_B * sizePart_B + 1.0)  * ((double)cutPart_B + sizePart_B);

    for (int cluster1 : this->cluster_S) {
        sizePart_S += streamCluster->getEdgeNum(cluster1, cluster1);
        for (int cluster2 : this->cluster_S) {
            int innerCut = 0;
            if (cluster1 != cluster2) {
                innerCut = streamCluster->getEdgeNum(cluster1, cluster2);
                this->newClusterNeighbours[cluster1] += innerCut;
            }
            
            cutPart_S += innerCut;
            auto it = cutCostValue.find(cluster1);
            if (it == cutCostValue.end())
                cutCostValue[cluster1 ] = 0;
            cutCostValue[cluster1] += innerCut;
        }

        for (int cluster2 : this->cluster_B) {
            int innerCut = 0;
            if (cluster1 != cluster2) {
                innerCut = streamCluster->getEdgeNum(cluster2,  cluster1);
                this->newClusterNeighbours[cluster1] += innerCut;
            }
            if(cutCostValue.find(cluster1) == cutCostValue.end()) {
                cutCostValue[cluster1]  = 0;
            }
            cutCostValue[cluster1] += innerCut;
        }
    }
    this->beta_S = (double)this->streamCluster->config.partitionNum  / (sizePart_S * sizePart_S + 1.0)  * ((double)cutPart_S + sizePart_S);
    // ================start game====================
    bool finish_B = false;bool finish_S = false;bool isChangeB = true;bool isChangeS = true;
    int times = 0;
    while (true) {
        finish_B = true;
        finish_S = true;
        for (int clusterId : this->cluster_B) {
            double minCost = std::numeric_limits<double>::max();
            int minPartition = (*clusterPartition)[clusterId];
            for (int j = 0; j < config->partitionNum; j++) {
                double cost = computeCost(clusterId, j, "B");
                if (cost <= minCost) {
                    minCost = cost;
                    minPartition = j;
                }
            }

            if (minPartition != (*clusterPartition)[clusterId]) {
                finish_B = false;
                partitionLoad[minPartition] += streamCluster->getEdgeNum(clusterId, clusterId);
                partitionLoad[(*clusterPartition)[clusterId]] -= streamCluster->getEdgeNum(clusterId, clusterId);
                (*clusterPartition)[clusterId] = minPartition;
            }
        }

        for (int clusterId : this->cluster_S) {
            double minCost = std::numeric_limits<double>::max();
            int minPartition = (*clusterPartition)[clusterId];
            for (int j = config->partitionNum - 1; j >= 0; j--) {
                double cost = computeCost(clusterId, j, "S");
                if (cost <= minCost) {
                    minCost = cost;
                    minPartition = j;
                }
            }

            if (minPartition != (*clusterPartition)[clusterId]) {
                finish_S = false;
                partitionLoad[minPartition] += streamCluster->getEdgeNum(clusterId, clusterId);
                partitionLoad[(*clusterPartition)[clusterId]] -= streamCluster->getEdgeNum(clusterId, clusterId);
                (*clusterPartition)[clusterId] = minPartition;
            }
        }
        roundCnt++;
        if (finish_B && finish_S) {break;}
    }

}

double ClusterGameTask::computeCost(int clusterId, int partition, const std::string type) {

    if (type == "B") {
        double loadPart = 0.0;
        double edgeCutPart = cutCostValue[clusterId];
        int old_partition = (*clusterPartition)[clusterId];
        loadPart = partitionLoad[old_partition];
        if (partition != old_partition)
            loadPart = partitionLoad[partition] + streamCluster->getEdgeNum(clusterId, clusterId);
        edgeCutPart -= this->newClusterNeighbours[clusterId];
        double alpha = config->alpha, k = config->partitionNum;
        double m = streamCluster->getEdgeNum(clusterId, clusterId);
        double Cost = beta_B / k * loadPart * m +  edgeCutPart   + m;
        return Cost;
    } else if (type == "S") {
        double loadPart = 0.0;
        double edgeCutPart =  cutCostValue[clusterId];
        int old_partition = (*clusterPartition)[clusterId];
        loadPart = partitionLoad[old_partition];
        if (partition != old_partition)
            loadPart = partitionLoad[partition] + streamCluster->getEdgeNum(clusterId, clusterId);
        edgeCutPart -= this->newClusterNeighbours[clusterId];
        double alpha = config->alpha, k = config->partitionNum;
        double m = streamCluster->getEdgeNum(clusterId, clusterId);
        double Cost = beta_S / k * loadPart * m + edgeCutPart  +  m;
        return Cost;
    } else {
        double loadPart = 0.0;
        double edgeCutPart = cutCostValue[clusterId];
        int old_partition = (*clusterPartition)[clusterId];
        loadPart = partitionLoad[old_partition];
        if (partition != old_partition)
            loadPart = partitionLoad[partition] + streamCluster->getEdgeNum(clusterId, clusterId);
        edgeCutPart -= this->newClusterNeighbours[clusterId];
        double alpha = config->alpha, k = config->partitionNum;
        double m = streamCluster->getEdgeNum(clusterId, clusterId);
        double Cost = beta / k * loadPart * m +  edgeCutPart   + m;
        return Cost;
    }
    return -1;
}